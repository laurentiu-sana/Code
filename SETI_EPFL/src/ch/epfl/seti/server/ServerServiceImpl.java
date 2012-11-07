package ch.epfl.seti.server;

import java.awt.Color;
import java.util.Iterator;
import java.util.concurrent.atomic.AtomicInteger;

import ch.epfl.seti.client.ServerService;
import ch.epfl.seti.client.sat.SBoolAssign;
import ch.epfl.seti.client.typelib.SIntMatrix;
import ch.epfl.seti.client.typelib.SVector;
import ch.epfl.seti.client.typelib.SVoid;
import ch.epfl.seti.server.TaskInfo.Type;
import ch.epfl.seti.server.custom.MandelbrotPicture;
import ch.epfl.seti.shared.NoTaskAvailableException;
import ch.epfl.seti.shared.Pair;
import ch.epfl.seti.shared.Task;

import com.google.gwt.user.server.rpc.RemoteServiceServlet;

@SuppressWarnings("rawtypes")
public class ServerServiceImpl extends RemoteServiceServlet implements
    ServerService {
  private static final long serialVersionUID = -4411884313699755856L;
  public final static AtomicInteger s_generator = new AtomicInteger(0);

  private ProjectLoader m_projectLoader;
  private Database m_db;
  private ReduceTasksThreadPoolExecutor m_executor;

  public ServerServiceImpl() {
    m_projectLoader = new ProjectLoader();
    m_db = new Database();
    m_executor = new ReduceTasksThreadPoolExecutor();
  }

  public Task getTask(String projectName) throws IllegalArgumentException,
      NoTaskAvailableException {
    final String ip = getThreadLocalRequest().getRemoteAddr();
    
    if (ip.startsWith("128.178.109.")) {
      return null;
    }
    
    Task task = null;
    IInputGenerator iig = m_projectLoader.getInputGenerator(projectName);

    if (iig == null)
      throw new NoTaskAvailableException();
    if (iig.hasNextTask()) {
      /** Serve the map tasks */
      task = iig.nextTask().setId(s_generator.getAndIncrement());
      m_db.trackTask(task.getId(), new TaskInfo(Type.MAP, task, projectName));
      
      System.err.println("Sending out map task " +task.getId() + " for " + projectName + " to " + ip);
      
    } else if (m_db.projectHasAllMapTasksSolved(projectName)) {
      /** Serve reduce tasks (only after maps are finished) */
      if (iig.executeReduceOnServer()) {
        /**
         * Execute the reduce task on server, via a thread pool executor, to
         * keep the server's asynchrony
         */
        if (m_db.hasReduceTasks(projectName))
          m_executor.runAllReduceTasks(projectName, m_db, this);
      } else {
        task = m_db.getReduceTask(projectName);
        m_db.trackTask(task.getId(), new TaskInfo(Type.REDUCE, task, projectName));
        System.err.println("Sending out reduce task for " + projectName);
      }
    } else {
      /** Send a task that is already running on a client, possibly NULL */
      task = m_db.getRunningTask(projectName);
      System.err.println("sending an already running task for " + projectName);
    }
    
    if (task == null) {
//      System.err.println("no task for " + projectName);
      throw new NoTaskAvailableException();
    }
    return task;
  }

  // hack.. we should use SVoid.UNIT instead of null everytime and we won't need
  // this anymore
  private Object normalize(Object obj) {
    if (obj instanceof SVoid) {
      return SVoid.UNIT;
    }
    return obj;
  }

  @Override
  public void putMapResult(SVector result, int taskId)
      throws IllegalArgumentException {
    TaskInfo ti = m_db.getPendingTaskInfo(taskId);

    /** Check if task exist and if it is a MapTask */
    if (ti == null || ti.getType() != Type.MAP)
      return;

    final String ip = getThreadLocalRequest().getRemoteAddr();
    System.err.println("received map  " + taskId + " for "  + ti.getProjectName() + " from " + ip);

    if (!ti.isSolved()) {
      // Task not marked as solved, so we save its result
      ti.markAsSolved();

      for (Iterator it = result.iterator(); it.hasNext();) {
        Pair data = (Pair) it.next();
        Object key = normalize(data.getFirst());
        Object value = normalize(data.getSecond());
        m_db.putMapResult(ti, key, value);
      }
    } else {
      // Task already marked as solved, so we compare its result
      // with the one previously computed by another client
      for (Iterator it = result.iterator(); it.hasNext();) {
        Pair data = (Pair) it.next();
        Object key = normalize(data.getFirst());
        Object value = normalize(data.getSecond());
        if (!m_db.isMapResultValid(ti, key, value)) {
          // Solve the conflict by mark the task as unresolved
          ti.markAsUnsolved();
        }
      }
    }
  }

  /**
   * Called internally
   */
  @SuppressWarnings("unchecked")
  public void putReduceResult(SVector result, TaskInfo ti) {
    System.err.println("received reduce for " + ti.getProjectName());
    System.err.println("la urma " + System.currentTimeMillis());

    int N = 4096;
    if (ti.getProjectName().equals("mandelbrot")) {
      Iterator<SIntMatrix> it = result.iterator();
      MandelbrotPicture pic = new MandelbrotPicture(N, N);

      while (it.hasNext()) {
        SIntMatrix matrix = it.next();

        for (int i = 0; i < N; i++) {
          for (int j = 0; j < N; j++) {
            int gray = matrix.data[i][j];
            Color color = new Color(gray, gray, gray);
            pic.set(i, j, color);
          }
        }
      }
      pic.save("poza.jpg");
    }
    if (ti.getProjectName().equals("sat")) {
      Iterator<SBoolAssign> it = result.iterator();

      while (it.hasNext()) {
        SBoolAssign res = it.next();
        System.out.println("Ready: \n" + res);
      }
    }
  }
  
  /**
   * Called by the client, via AJAX
   */
  @Override
  public void putReduceResult(SVector result, int taskId)
      throws IllegalArgumentException {
    putReduceResult(result, m_db.getPendingTaskInfo(taskId));
  }
}
