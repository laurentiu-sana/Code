package ch.epfl.seti.client.sat;

import com.google.gwt.user.client.rpc.IsSerializable;

//Atom of type : x1  or not(x2) 
public class Atom implements IsSerializable {
  private String name;
  private boolean neg; // true if atom is neg(v)

  public Atom(String v, boolean neg) {
    this.name = v;
    this.neg = neg;
  }

  public Atom() {

  }

  public String getName() {
    return name;
  }

  public boolean getNeg() {
    return neg;
  }

  public String toString() {
    if (neg)
      return "not(" + name + ")";
    return name;
  }
}
