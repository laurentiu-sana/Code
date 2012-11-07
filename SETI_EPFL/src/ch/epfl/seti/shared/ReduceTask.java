package ch.epfl.seti.shared;

import ch.epfl.seti.client.typelib.SVector;

import com.google.gwt.user.client.rpc.IsSerializable;

public class ReduceTask<K extends IsSerializable, V extends IsSerializable>
		extends Pair<K, SVector<V>> implements Task<K, V>{
  private int id = -1;

	public ReduceTask() {
	}
	
	public ReduceTask(K k, SVector<V> vs) {
	  super(k, vs);
	}

  @Override
  public int getId() {
    return id;
  }
  
  public ReduceTask<K, V> setId(int id) {
    this.id = id;
    return this;
  }


}
