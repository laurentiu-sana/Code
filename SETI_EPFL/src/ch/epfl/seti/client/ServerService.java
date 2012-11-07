package ch.epfl.seti.client;

import ch.epfl.seti.client.typelib.SVector;
import ch.epfl.seti.shared.NoTaskAvailableException;
import ch.epfl.seti.shared.Pair;
import ch.epfl.seti.shared.Task;

import com.google.gwt.user.client.rpc.IsSerializable;
import com.google.gwt.user.client.rpc.RemoteService;
import com.google.gwt.user.client.rpc.RemoteServiceRelativePath;

/**
 * The client side stub for the RPC service.
 */
@RemoteServiceRelativePath("server")
public interface ServerService<K1 extends IsSerializable, V1 extends IsSerializable, K2 extends IsSerializable, V2 extends IsSerializable> extends RemoteService {
  @SuppressWarnings("rawtypes")
  Task getTask(String projectName) throws IllegalArgumentException, NoTaskAvailableException;
	
	void putMapResult(SVector<Pair<K2, V2>> result, int taskId) throws IllegalArgumentException;
	
	void putReduceResult(SVector<V2> result, int taskId) throws IllegalArgumentException;
}
