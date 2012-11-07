package ch.epfl.seti.client.sat;

import com.google.gwt.user.client.rpc.IsSerializable;

//Triplet: atom(x1, true) || atom(x2, false) || atom(x3,true)
public class Triplet implements IsSerializable {
  @SuppressWarnings("unused")
  private static final long serialVersionUID = -8971882494109192443L;
  private Atom[] a = new Atom[3];
  int pos = 0;

  public Triplet(Atom a1, Atom a2, Atom a3) {
    a[0] = a1;
    a[1] = a2;
    a[2] = a3;
  }

  public Triplet() {
    super();
  }

  public void add(Atom at) {
    a[pos++] = at;
  }

  public Atom elementAt(int idx) {
    return a[idx];
  }

  public String toString() {
    return elementAt(0).toString() + " OR " + elementAt(1).toString() + " OR "
        + elementAt(2).toString();
  }
}
