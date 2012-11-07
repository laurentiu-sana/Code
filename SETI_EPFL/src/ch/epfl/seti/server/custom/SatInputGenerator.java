package ch.epfl.seti.server.custom;

import java.util.concurrent.atomic.AtomicInteger;

import ch.epfl.seti.client.sat.SBoolAssign;
import ch.epfl.seti.client.sat.SExpression;
import ch.epfl.seti.client.sat.SPartialyAssignedExpression;
import ch.epfl.seti.client.typelib.SVoid;
import ch.epfl.seti.server.IInputGenerator;
import ch.epfl.seti.shared.MapTask;
import ch.epfl.seti.shared.Pair;
import ch.epfl.seti.shared.Task;

public class SatInputGenerator implements IInputGenerator<SPartialyAssignedExpression, SVoid>{
  public static SExpression e;
  private AtomicInteger m_counter = new AtomicInteger(0); 
  private int number_tasks = 64; // CHANGE HERE NUMBER OF TASKS!!
  int nr_partial_vars = -1;  // CHANGE HERE TO -2 if you want for any task to be executed
  								// by 3 or 4 workers (ERROR tolerance)
  
  public SatInputGenerator(){
	  /*
	  String expr = "(x1 | x1 | x1) & (not x1 | not x1 | not x1) ";
	  for(int i = 1 ; i <= 6; i ++) {
		  expr = expr + "& (x" + (3*i - 1) + " | x" + (3*i) + " | x" + (3*i + 1) + ") "; 
	  }
	  */
	  int number_of_variables = 24; // CHANGE HERE !
	  String expr = "(not x1 | not x1 | not x1) ";
	  for(int i = 2 ; i <= number_of_variables; i ++) {
		  expr = expr + "& (not x" + i + " | not x" + i + " | not x" + i + ") "; 
	  }
	  
	  e = new SExpression(expr);		  
	  System.out.println(e);
	  System.out.println(e.varsList());

	  // number of assigned partial vars
	  int prod = 1;
	  while(prod <= number_tasks){
		  nr_partial_vars ++;
		  prod = prod * 2;
	  }
  }
  
  @Override
  public boolean hasNextTask() {
    return (m_counter.get() < number_tasks);
  }

  // not only 2 tasks
  @Override
  public Task<SPartialyAssignedExpression, SVoid> nextTask() {
    int taskNo = m_counter.getAndIncrement();
    if (taskNo >= number_tasks) {
      m_counter.decrementAndGet();
      return null;
    }
    
    SBoolAssign partial = new SBoolAssign();
    int currentNo = taskNo;
    for (int i = 0; i < nr_partial_vars; i++, currentNo = currentNo >> 1 )
    	if(currentNo%2 == 0)
    		partial.put(e.varsList().elementAt(i), false);
    	else
    		partial.put(e.varsList().elementAt(i), true);
    		
   	MapTask<SPartialyAssignedExpression,SVoid> ret = new MapTask<SPartialyAssignedExpression, SVoid>();
    ret.add(new Pair<SPartialyAssignedExpression, SVoid>(new SPartialyAssignedExpression(e,partial), SVoid.UNIT));
    return ret;
  }

  @Override
  public boolean executeReduceOnServer() {
    return true;
  }
}
