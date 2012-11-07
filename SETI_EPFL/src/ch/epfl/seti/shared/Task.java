package ch.epfl.seti.shared;

import com.google.gwt.user.client.rpc.IsSerializable;

public interface Task<K extends IsSerializable, V extends IsSerializable> extends IsSerializable {
  /**
   * This ID is an identifier of a task. 
   * 
   * TODO: for security, this ID should be some UUID, such that clients are not able to create a fake one.
   */
	public int getId();
	
	public Task<K, V> setId(int i);
	
}
