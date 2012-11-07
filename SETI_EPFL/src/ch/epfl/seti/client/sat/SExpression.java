package ch.epfl.seti.client.sat;

import java.util.Vector;

import ch.epfl.seti.client.typelib.SVector;

import com.google.gwt.user.client.rpc.IsSerializable;



public class SExpression extends SVector<Triplet> implements IsSerializable {
  private static final long serialVersionUID = -5527124747815411552L;

  public Vector<String> split(String text, String delims) {
    Vector<String> res = new Vector<String>();
    String current = "";
    for (int i = 0; i < text.length(); i++)
      if (delims.indexOf(text.charAt(i)) == -1) {
        current = current + text.charAt(i);
      } else {
        if (current.length() > 0)
          res.add(current);
        current = "";
      }
    return res;
  }

  public SExpression(String text) {
    super();

    Vector<String> tokens = split(text, " ()|&");
    Triplet tr = new Triplet();
    int size_tr = 0;
    for (int i = 0; i < tokens.size(); i++) {
      String token = tokens.elementAt(i);
      if (token.compareTo("not") == 0) {
        i++;
        tr.add(new Atom(tokens.elementAt(i), true));
      } else {
        tr.add(new Atom(token, false));
      }
      size_tr++;
      if (size_tr == 3) {
        this.add(tr);
        tr = new Triplet();
        size_tr = 0;
      }
    }
  }

  public SExpression() {
  }

  public String toString() {
    String res = "";
    if (this.size() == 0)
      return "";
    for (int i = 0; i < this.size() - 1; i++)
      res = res + "(" + this.elementAt(i).toString() + ") AND ";
    return res + "(" + this.elementAt(this.size() - 1).toString() + ")";
  }

  public Vector<String> varsList() {
    Vector<String> res = new Vector<String>();
    for (int i = 0; i < this.size(); i++) {
      for (int j = 0; j < 3; j++) {
        if (!res.contains(this.elementAt(i).elementAt(j).getName()))
          res.add(this.elementAt(i).elementAt(j).getName());
      }
    }
    return res;
  }

  public boolean eval(SBoolAssign assigned) {
    boolean res = true;
    for (int i = 0; i < this.size(); i++) {
      boolean temp = false;
      for (int j = 0; j < 3; j++)
        if (this.elementAt(i).elementAt(j).getNeg())
          temp |= !assigned.get(this.elementAt(i).elementAt(j).getName());
        else
          temp |= assigned.get(this.elementAt(i).elementAt(j).getName());
      res &= temp;
    }
    return res;
  }
}
