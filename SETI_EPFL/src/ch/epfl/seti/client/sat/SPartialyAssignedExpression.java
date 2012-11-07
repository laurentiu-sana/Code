package ch.epfl.seti.client.sat;

import com.google.gwt.user.client.rpc.IsSerializable;
import java.util.*;
import java.io.*;

public class SPartialyAssignedExpression implements IsSerializable{
	public SExpression e;
	public SBoolAssign partialAssignment;
	
	public SPartialyAssignedExpression(SExpression e, SBoolAssign partialAssignment){
		this.e = e;
		this.partialAssignment = partialAssignment;
	}
	
	public SPartialyAssignedExpression() {
  }

	@Override
	public String toString(){
		return e.toString() + ";" + partialAssignment.toString();
	}
	
	@Override
	public boolean equals(Object obj) {
	  if (obj instanceof SPartialyAssignedExpression) {
	    return ((SPartialyAssignedExpression) obj).e == e && 
	    		((SPartialyAssignedExpression) obj).partialAssignment == partialAssignment;
	  }
	  return false;
	}	
}