package ch.epfl.seti.client;

import ch.epfl.seti.shared.Pair;

import com.google.gwt.user.client.rpc.IsSerializable;

public interface IMapWorker<K1 extends IsSerializable, V1 extends IsSerializable, K2 extends IsSerializable, V2 extends IsSerializable> {
	public void map(K1 key, V1 value, IOutputCollector<Pair<K2, V2>> output);
}
