package ch.epfl.seti.server;

import java.io.ByteArrayInputStream;
import java.io.ByteArrayOutputStream;
import java.io.IOException;
import java.io.ObjectInputStream;
import java.io.ObjectOutputStream;
import java.util.HashMap;
import java.util.HashSet;
import java.util.Iterator;
import java.util.NavigableMap;
import java.util.concurrent.ArrayBlockingQueue;
import java.util.concurrent.ThreadPoolExecutor;
import java.util.concurrent.TimeUnit;

import org.apache.hadoop.hbase.client.Result;
import org.apache.hadoop.hbase.client.ResultScanner;
import org.apache.hadoop.hbase.client.Scan;
import org.apache.hadoop.hbase.util.Bytes;

import ch.epfl.seti.client.typelib.SVector;
import ch.epfl.seti.server.TaskInfo.Type;
import ch.epfl.seti.shared.ReduceTask;
import ch.epfl.seti.shared.Task;

import com.google.gwt.user.client.rpc.IsSerializable;

public class Database {
  private HashMap<Integer, TaskInfo> m_tasks = new HashMap<Integer, TaskInfo>();

  @SuppressWarnings("rawtypes")
  private HashMap<Object, SVector> m_reduceTasks = new HashMap<Object, SVector>();
  private final static HBaseWrapper s_wrapper = HBaseWrapper
      .createHBaseConnection();
  private static ByteArrayOutputStream s_bos = null;
  private static ObjectOutputStream s_oos = null;

  static {
    try {
      s_bos = new ByteArrayOutputStream();
      s_oos = new ObjectOutputStream(s_bos);
    } catch (IOException e) {
      e.printStackTrace();
    }
  }

  public Database() {
    restoreState();
    initCheckpointExecutor();
  }

  public void trackTask(int id, TaskInfo ti) {
    m_tasks.put(id, ti);
    checkpointNewChange(ti.getProjectName(), "task_id" + id, ti);
  }

  public TaskInfo getPendingTaskInfo(int id) {
    return m_tasks.get(id);
  }

  /** Checks if a project has all the Map tasks solved */
  public boolean projectHasAllMapTasksSolved(String projectName) {
    boolean result = true;

    for (TaskInfo ti : m_tasks.values()) {
      /** For-each task belonging to the project */
      if (ti.getProjectName().equals(projectName)) {
        /** Check if the map task is solved */
        if (ti.getType() == Type.MAP && !ti.isSolved())
          return false;
      }
      return true;
    }

    return result;
  }

  interface TaskInfoPredicate {
    public boolean isValid(TaskInfo ti);
  }

  @SuppressWarnings("rawtypes")
  private Task getTaskSatisfyingPredicate(String projectName, Type type,
      TaskInfoPredicate predicate) {
    /**
     * Use timestamp to pick the best task satisfying a given predicate; we pick
     * the oldest task and, at the end, we update its timestamp
     */
    TaskInfo bestTaskInfo = null;
    long currentTime = System.currentTimeMillis();
    long minTime = currentTime;

    for (TaskInfo ti : m_tasks.values()) {
      /** For-each task belonging to the project */
      if (ti.getProjectName().equals(projectName)) {
        /** Check if the map task is solved */
        if (ti.getType() == type) {
          assert ti.getTask() != null;
          if (predicate.isValid(ti)) {
            if (ti.getLastTimestamp() <= minTime) {
              minTime = ti.getLastTimestamp();
              bestTaskInfo = ti;
            }
          }
        }
      }
    }

    if (bestTaskInfo != null) {
      bestTaskInfo.setLastTimestamp(currentTime);
      return bestTaskInfo.getTask();
    }

    return null;
  }

  @SuppressWarnings("rawtypes")
  private Task getTaskSatisfyingPredicate(String projectName,
      TaskInfoPredicate predicate) {
    Task result;
    /** Search for a map task that is late */
    result = getTaskSatisfyingPredicate(projectName, Type.MAP, predicate);
    if (result == null) {
      /**
       * If the no map task is late, then look for a late reduce task only if
       * the map tasks are finished
       */
      if (projectHasAllMapTasksSolved(projectName)) {
        result = getTaskSatisfyingPredicate(projectName, Type.REDUCE, predicate);
      }
    }
    return result;
  }

  /**
   * Returns any task that is late; the Map tasks are considered first We are
   * using this function to implement the speculative execution, when two
   * clients receive the same task to be solved.
   */
  @SuppressWarnings("rawtypes")
  public Task getRunningTask(String projectName) {
    Task result;

    /** Search for a late task */
    result = getTaskSatisfyingPredicate(projectName, new TaskInfoPredicate() {
      @Override
      public boolean isValid(TaskInfo ti) {
        return ti.isLate() && !ti.isSolved();
      }
    });
    if (result == null) {
      /** If no late task is found, then return any non-solved task */
      result = getTaskSatisfyingPredicate(projectName, new TaskInfoPredicate() {
        @Override
        public boolean isValid(TaskInfo ti) {
          return !ti.isSolved();
        }
      });
    }
    return result;
  }

  @SuppressWarnings({ "rawtypes", "unchecked" })
  public ReduceTask getReduceTask(String projectName) {

    Iterator<Object> it = m_reduceTasks.keySet().iterator();
    if (it.hasNext()) {
      IsSerializable key = (IsSerializable) it.next();
      SVector values = m_reduceTasks.get(key);
      m_reduceTasks.remove(key);
      return new ReduceTask(key, values).setId(ServerServiceImpl.s_generator
          .getAndIncrement());
    }
    return null;
  }

  public boolean hasReduceTasks(String projectName) {
    return !m_reduceTasks.keySet().isEmpty();
  }

  /**
   * Store the result of a map.
   */
  @SuppressWarnings({ "rawtypes", "unchecked" })
  public void putMapResult(TaskInfo ti, Object key, Object value) {
    SVector list = null;
    if (!m_reduceTasks.containsKey(key)) {
      m_reduceTasks.put(key, new SVector());
    }
    list = m_reduceTasks.get(key);
    list.add(value);
    checkpointNewChange(ti.getProjectName(), key, value);
  }

  @SuppressWarnings({ "rawtypes" })
  public boolean isMapResultValid(TaskInfo ti, Object key, Object value) {
    boolean result = false;
    if (m_reduceTasks.containsKey(key)) {
      SVector list = m_reduceTasks.get(key);
      result = list.contains(value);
    }
    return result;
  }

  private ThreadPoolExecutor m_threadPool = null;


  private void initCheckpointExecutor() {
    int m_poolSize = 4;
    int m_maxPoolSize = 4;
    long m_keepAliveTime = 60;
    int m_queueSize = 1024;

    final ArrayBlockingQueue<Runnable> m_queue = new ArrayBlockingQueue<Runnable>(m_queueSize);
    m_threadPool = new ThreadPoolExecutor(m_poolSize, m_maxPoolSize, m_keepAliveTime,
        TimeUnit.SECONDS, m_queue);
  }

  
  private void checkpointNewChange(final String projectName, final Object key, final Object value) {
    m_threadPool.execute(new Runnable() {
    @Override
    public void run() {
      // TODO Auto-generated method stub
      try {
        byte[] akey;
        s_oos.writeObject(key);
        akey = s_bos.toByteArray();
        s_oos.reset();
        s_bos.reset();

        byte[] avalue;
        s_oos.writeObject(value);
        avalue = s_bos.toByteArray();

        /** Storing the actual object */
        s_wrapper.putData(projectName, akey, avalue);

        s_oos.reset();
        s_bos.reset();
      } catch (Exception e) {
        e.printStackTrace();
      }
    }
    });
  }

  @SuppressWarnings({"rawtypes", "unchecked"})
  private void restoreState() {
    /** XXX: link the the projects from the ProjectLoader class */
    HashMap<String, IInputGenerator> projects = new HashMap<String, IInputGenerator>();

    try {
      for (String projectName : projects.keySet()) {
        Scan scan = new Scan(Bytes.toBytes(projectName));
        ResultScanner scanner = s_wrapper.getHTable().getScanner(scan);
        for (Result result : scanner) {
          NavigableMap<byte[], NavigableMap<byte[], NavigableMap<Long, byte[]>>> map = result
              .getMap();
          for (byte[] it : map.keySet()) {
            NavigableMap<byte[], NavigableMap<Long, byte[]>> data = map.get(it);
            for (byte[] key : data.keySet()) {
              NavigableMap<Long, byte[]> values = data.get(key);
              for (Long timestamp : values.keySet()) {
                byte[] value = values.get(timestamp);
                Object o_key = new ObjectInputStream(new ByteArrayInputStream(
                    key)).readObject();
                Object o_value = new ObjectInputStream(
                    new ByteArrayInputStream(value)).readObject();

                /** Restore the results */
                if (!m_reduceTasks.containsKey(key)) {
                  m_reduceTasks.put(key, new SVector());
                }
                SVector list = m_reduceTasks.get(o_key);
                list.add(o_value);
              }
            }
          }
        }
      }
    } catch (Exception e) {
      e.printStackTrace();
    }
  }
}
