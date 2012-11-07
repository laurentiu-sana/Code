package ch.epfl.seti.server;

import java.util.HashMap;
import java.util.HashSet;
import java.util.concurrent.ArrayBlockingQueue;
import java.util.concurrent.ThreadPoolExecutor;
import java.util.concurrent.TimeUnit;

import ch.epfl.seti.client.IOutputCollector;
import ch.epfl.seti.client.IReduceWorker;
import ch.epfl.seti.client.typelib.SVector;
import ch.epfl.seti.server.TaskInfo.Type;
import ch.epfl.seti.shared.ReduceTask;
import ch.epfl.seti.shared.Task;

import com.google.gwt.user.client.rpc.IsSerializable;

/** Thread pool used by the server to run the reduce tasks that shouldn't be
 * relocated on clients, for various reasons (e.g. data transfer cost is
 * comparable to the amount of computation needed by the reduce operation)
 */
public class ReduceTasksThreadPoolExecutor {
  int m_poolSize = 4;
  int m_maxPoolSize = 4;
  long m_keepAliveTime = 60;
  int m_queueSize = 1024;
  private HashSet<String> m_completedProjects;

  ThreadPoolExecutor m_threadPool = null;

  final ArrayBlockingQueue<Runnable> m_queue = new ArrayBlockingQueue<Runnable>(
      m_queueSize);

  public ReduceTasksThreadPoolExecutor() {
    m_completedProjects = new HashSet<String>();
    m_threadPool = new ThreadPoolExecutor(m_poolSize, m_maxPoolSize,
        m_keepAliveTime, TimeUnit.SECONDS, m_queue);
  }
  
  @SuppressWarnings("rawtypes")
  class ThreadPoolReduceOutputCollector implements IOutputCollector {
    private SVector m_data = new SVector();
    
    @SuppressWarnings("unchecked")
    @Override
    public void emit(IsSerializable data) {
      m_data.add(data);
    }
    
    public SVector getCollectedData() {
      return m_data;
    }
  };

  
  @SuppressWarnings("rawtypes")
  public boolean runTask(final String projectName, Task task, final ServerServiceImpl server) {
    boolean result = false;
    if (task != null) {
      /** Load the project's ReduceWoker */
      Object worker = ProjectLoader.loadUserDefinedReduceWorker(projectName);
      if (worker != null) {
        final IReduceWorker reducer = (IReduceWorker) worker;
        final ReduceTask reduceTask = (ReduceTask) task;
        final ThreadPoolReduceOutputCollector oc = new ThreadPoolReduceOutputCollector();
        
        /** Create a task that would eventually reduce our current task */
        m_threadPool.execute(new Runnable() {
          @SuppressWarnings("unchecked")
          @Override
          public void run() {
            /** Let the user's reducer do the job */
            reducer.reduce(reduceTask.getFirst(), (SVector) reduceTask.getSecond(), oc);
            
            /** Announce the server that we finished to process this reduce task */
            TaskInfo ti = new TaskInfo(Type.REDUCE, reduceTask, projectName);
            server.putReduceResult(oc.getCollectedData(), ti);
          }
        });
        result = true;
      }
    }
    return result;
  }
  
  /**
   * Run all reduce tasks on the thread pool.
   */
  @SuppressWarnings("rawtypes")
  public boolean runAllReduceTasks(final String projectName,final Database db, final ServerServiceImpl server){
    if (m_completedProjects.contains(projectName))
      return false;
    
    m_threadPool.execute(new Runnable() {
      @Override
      public void run() {
        ReduceTask task = null;
        while ((task = db.getReduceTask(projectName))!= null) {
          ReduceTasksThreadPoolExecutor.this.runTask(projectName, task, server);
        }
      }
    });
    m_completedProjects.add(projectName);
    return true;
  }
}
