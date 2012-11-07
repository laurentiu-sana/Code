package ch.epfl.seti.client.custom;

import ch.epfl.seti.client.IOutputCollector;
import ch.epfl.seti.client.IReduceWorker;
import ch.epfl.seti.client.sat.SBoolAssign;
import ch.epfl.seti.client.typelib.*;

public class SatReduceWorker implements IReduceWorker<SVoid, SBoolAssign> {

  @Override
  public void reduce(SVoid key, SVector<SBoolAssign> value,
      IOutputCollector<SBoolAssign> output) {
	  for(int i = 0; i < value.size(); i++)
		  if(value.elementAt(i) != null){
			  output.emit(value.elementAt(i));
			  return;
		  }
	  output.emit(null);
  }

}
