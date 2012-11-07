package ch.epfl.seti.server;

import java.io.Serializable;
import java.util.concurrent.atomic.AtomicBoolean;

import ch.epfl.seti.shared.Constants;
import ch.epfl.seti.shared.Task;


/**
 * All informations that we keep about a task that is in execution at the client
 * (e.g. its type, how many times it executed if we execute it 3 times, the 
 * client that is executing it etc.)
 */
@SuppressWarnings("rawtypes")
public class TaskInfo implements Serializable {
  private static final long serialVersionUID = 1L;

enum Type { MAP, REDUCE };
  private Type m_type;
  private Task m_task;
  private String m_projectName;
  private AtomicBoolean m_solved;
  private long m_timestamp;
  private long m_lastTimestamp;
  
  public TaskInfo(Type type, Task task, String projectName) {
    m_type = type;
    m_task = task;
    m_projectName = projectName;
    m_solved = new AtomicBoolean(false);
    m_timestamp = System.currentTimeMillis();
    m_lastTimestamp = m_timestamp;
  }
  
  public Type getType() {
    return m_type;
  }
  
  public Task getTask() {
    return m_task;
  }
  
  public String getProjectName() {
    return m_projectName;
  }
  
  public boolean isSolved() {
    return m_solved.get();
  }
  
  public void markAsSolved() {
    m_solved.compareAndSet(false, true);
  }
  
  public void markAsUnsolved() {
    m_solved.compareAndSet(true, false);
  }
  
  public boolean isLate() {
    return System.currentTimeMillis() - m_timestamp > Constants.TASK_TIMEOUT;
  }
  
  public long getLastTimestamp() {
    return m_lastTimestamp;
  }
  
  public void setLastTimestamp(long timestamp) {
    m_lastTimestamp = timestamp;
  }
}
