package ch.epfl.seti.server;

import java.util.HashMap;

import com.google.gwt.user.client.rpc.IsSerializable;

/**
 * A class with all the information about a particular project
 */
public class ProjectLoader {
  // The Java packet in which all the input generators are compiled 
  private final static String INPUT_GENERATOR_PACKAGE = "ch.epfl.seti.server.custom";
  private final static String REDUCE_WORKER_PACKAGE = "ch.epfl.seti.client.custom";
  
  @SuppressWarnings("rawtypes")
  private HashMap<String, IInputGenerator> m_projects;

  @SuppressWarnings("rawtypes")
  public ProjectLoader() {
    m_projects = new HashMap<String, IInputGenerator>();
  }

  /** It might return NULL if the project's input generator class is not
   * defined or not present in the CLASSPATH
   */
  @SuppressWarnings("unchecked")
  public <K extends IsSerializable, V extends IsSerializable> IInputGenerator<K, V> getInputGenerator(
      String projectName) {
    if (!m_projects.containsKey(projectName)) {
      loadProject(projectName);
      System.err.println("la inceput " + System.currentTimeMillis());
    }
    return m_projects.get(projectName);
  }
  
  private static String normalizeProjectName(String projectName) {
    return Character.toUpperCase(projectName.charAt(0)) + projectName.substring(1);
  }
  
  @SuppressWarnings("rawtypes")
  private static Class loadClass(String className) {
    ClassLoader classLoader = ProjectLoader.class.getClassLoader();
    Class loadedClass = null;
    try {
      loadedClass = classLoader.loadClass(className);
    } catch (Exception e) {
      e.printStackTrace();
    }
    return loadedClass;
  }
  
  @SuppressWarnings("rawtypes")
  private static Object loadClassAndCreateNewInstance(String className) {
    Class lc = loadClass(className);
    Object result = null;
    try {
      result = lc.newInstance();
    } catch (Exception e) {
      e.printStackTrace();
    }
    return result;
  }
  
  @SuppressWarnings("unchecked")
  private void loadProject(String projectName) {
    // First letter to upper case
    String normProjectName = normalizeProjectName(projectName);
    String className = INPUT_GENERATOR_PACKAGE + "." + normProjectName + "InputGenerator";
    Object result = loadClassAndCreateNewInstance(className);
    if (result != null) {
      m_projects.put(projectName, (IInputGenerator<IsSerializable, IsSerializable>) result);
    }
  }
  
  public static Object loadUserDefinedReduceWorker(String projectName) {
    String normProjectName = normalizeProjectName(projectName);
    String className = REDUCE_WORKER_PACKAGE + "." + normProjectName + "ReduceWorker";
    return loadClassAndCreateNewInstance(className);
  }
}
