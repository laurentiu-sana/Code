package ch.epfl.seti.client.custom;

import java.util.Vector;

import ch.epfl.seti.client.IMapWorker;
import ch.epfl.seti.client.IOutputCollector;
import ch.epfl.seti.client.sat.SBoolAssign;
import ch.epfl.seti.client.sat.SExpression;
import ch.epfl.seti.client.sat.SPartialyAssignedExpression;
import ch.epfl.seti.client.typelib.SVoid;
import ch.epfl.seti.shared.Pair;


public class SatMapWorker implements
		IMapWorker<SPartialyAssignedExpression, SVoid, SVoid, SBoolAssign> {

  @Override
  public void map(SPartialyAssignedExpression key, SVoid value, IOutputCollector<Pair<SVoid, SBoolAssign>> output) {
    output.emit(new Pair<SVoid, SBoolAssign>(SVoid.UNIT, SATSolver.compute(key)));
  }
}

// SAT solver that uses backtracking.
class SATSolver {
	public static SBoolAssign compute(SPartialyAssignedExpression key) {
		return solve3SAT(key.e, key.e.varsList(), key.partialAssignment);
	}
	
	public static SBoolAssign solve3SAT(SExpression e, 
													 Vector<String> varlist, 
													 SBoolAssign assigned){
		if(varlist.size() == assigned.size()){
			//		System.out.println(assigned);
			if(e.eval(assigned))
				return assigned;			
			return null;
		}
		String next = varlist.elementAt(assigned.size());
		assigned.put(next, true);
		SBoolAssign h1 = solve3SAT(e, varlist, assigned);
		if(h1 != null)
			return h1;
		assigned.put(next, false);
		SBoolAssign h2 = solve3SAT(e, varlist, assigned);
		if(h2 != null)
			return h2;
		assigned.remove(next);
		return null;
	}	
}
