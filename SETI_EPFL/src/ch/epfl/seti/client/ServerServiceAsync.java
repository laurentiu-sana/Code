package ch.epfl.seti.client;

import ch.epfl.seti.client.typelib.SVector;
import ch.epfl.seti.shared.Pair;
import ch.epfl.seti.shared.Task;

import com.google.gwt.user.client.rpc.AsyncCallback;
import com.google.gwt.user.client.rpc.IsSerializable;

public interface ServerServiceAsync <K1 extends IsSerializable, V1 extends IsSerializable, K2 extends IsSerializable, V2 extends IsSerializable>{
	void getTask(String projectName, @SuppressWarnings("rawtypes") AsyncCallback<Task> callback) throws IllegalArgumentException;
	
	void putMapResult(SVector<Pair<K2, V2>> result, int taskId, AsyncCallback<Void> callback);
	
  void putReduceResult(SVector<V2> result, int taskId, AsyncCallback<Void> callback) throws IllegalArgumentException;
}
