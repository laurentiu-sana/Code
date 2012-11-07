package ch.epfl.seti.client.typelib;

import com.google.gwt.user.client.rpc.IsSerializable;

public class SInteger implements IsSerializable {
	public int i;

	public SInteger() {
		i = 0;
	}

	public SInteger(int i) {
		this.i = i;
	}
	
	@Override
	public String toString() {
		return i + "";
	}
	
	@Override
	public boolean equals(Object obj) {
	  if (obj instanceof SInteger) {
	    return ((SInteger) obj).i == i;
	  }
	  return false;
	}
}
