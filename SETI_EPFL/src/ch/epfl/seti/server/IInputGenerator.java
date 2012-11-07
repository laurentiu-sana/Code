package ch.epfl.seti.server;

import ch.epfl.seti.shared.Task;

import com.google.gwt.user.client.rpc.IsSerializable;

public interface IInputGenerator <K extends IsSerializable, V extends IsSerializable> {
  public boolean hasNextTask();
  public Task<K, V> nextTask();
  public boolean executeReduceOnServer();
}
