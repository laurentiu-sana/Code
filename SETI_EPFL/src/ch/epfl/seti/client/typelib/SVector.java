package ch.epfl.seti.client.typelib;

import java.util.Vector;

import com.google.gwt.user.client.rpc.IsSerializable;

public class SVector<T extends IsSerializable> extends Vector<T> implements IsSerializable {
  private static final long serialVersionUID = -7498591519735422083L;
  
  @Override
  public boolean equals(Object obj) {
    boolean result = false;
    if (obj instanceof SVector) {
      @SuppressWarnings("rawtypes")
      SVector vector = (SVector) obj;
      result = true;
      if (size() == vector.size()) {
        for (int i = 0; i < vector.size(); i++) {
          result &= get(i).equals(vector.get(i));
        }
      }
    }
    return result;
  }
}
