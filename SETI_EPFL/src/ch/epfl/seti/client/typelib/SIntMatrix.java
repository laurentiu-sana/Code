package ch.epfl.seti.client.typelib;

import com.google.gwt.user.client.rpc.IsSerializable;

public class SIntMatrix implements IsSerializable {
  public int data[][] = null;

  public SIntMatrix() {
  }

  public SIntMatrix(int m, int n) {
    data = new int[m][n];
  }
  
  @Override
  public String toString() {
    return data.length == 0 ? "0x0" : data.length + "x" + data[0].length;
  }
  
  @Override
  public boolean equals(Object obj) {
    boolean result = false;
    if (obj instanceof SIntMatrix) {
      SIntMatrix matrix = (SIntMatrix) obj;
      result = true;
      if (matrix.data.length == data.length) {
        for (int i = 0; i < data.length; i++) {
          if (matrix.data[i].length == data[i].length) {
            for (int j = 0; j < data[i].length; j++) {
              result &= matrix.data[i][j] == data[i][j];
            }
          }
        }
      }
    }
    return result;
  }
}
