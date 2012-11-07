package ch.epfl.seti.client;

import ch.epfl.seti.client.typelib.SVector;

import com.google.gwt.user.client.rpc.IsSerializable;

public interface IReduceWorker<K extends IsSerializable, V extends IsSerializable> {
  public void reduce(K key, SVector<V> value, IOutputCollector<V> output);
}
