package ch.epfl.seti.shared;

import java.util.Vector;

import com.google.gwt.user.client.rpc.IsSerializable;

public class MapTask<K extends IsSerializable, V extends IsSerializable>
		extends Vector<Pair<K, V>> implements Task<K,V>{
  private static final long serialVersionUID = 595409059919819223L;
  private int id = -1;
  
  public MapTask() {
  }
  
  @Override
  public int getId() {
    return id;
  }
  
  public MapTask<K, V> setId(int id) {
    this.id = id;
    return this;
  }
}
