package ch.epfl.seti.shared;

import com.google.gwt.user.client.rpc.IsSerializable;

public class Pair<T extends IsSerializable, U extends IsSerializable>
		implements IsSerializable {
	private T first;
	private U second;

	public Pair() {
	}

	public Pair(T f, U s) {
		this.first = f;
		this.second = s;
	}

	public T getFirst() {
		return first;
	}

	public U getSecond() {
		return second;
	}
	
	@Override
	public String toString() {
	  return "[" + first.toString() + ", " + second.toString() + "]";
	}
}