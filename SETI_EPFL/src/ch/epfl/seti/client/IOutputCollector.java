package ch.epfl.seti.client;

import com.google.gwt.user.client.rpc.IsSerializable;

public interface IOutputCollector<T extends IsSerializable> {
	public void emit(T data);
}
