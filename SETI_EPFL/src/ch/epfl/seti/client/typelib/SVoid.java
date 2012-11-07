package ch.epfl.seti.client.typelib;

import com.google.gwt.user.client.rpc.IsSerializable;

public class SVoid implements IsSerializable {
	public static SVoid UNIT = new SVoid();
	
	@Override
	public String toString() {
		return "void";
	}
	
	@Override
	public boolean equals(Object obj) {
	  return obj instanceof SVoid;
	}
}
