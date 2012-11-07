package ch.epfl.seti.client.custom;

import ch.epfl.seti.client.JobConf;
import ch.epfl.seti.client.typelib.SIntMatrix;
import ch.epfl.seti.client.typelib.SInteger;
import ch.epfl.seti.client.typelib.SVoid;

import com.google.gwt.core.client.EntryPoint;

/**
 * Entry point classes define <code>onModuleLoad()</code>.
 */

public class SETI_EPFL implements EntryPoint {
	public void onModuleLoad() {
	  JobConf<SInteger, SVoid, SVoid, SIntMatrix> conf = 
	      new JobConf<SInteger, SVoid, SVoid, SIntMatrix>();
	  conf.setMapper(new MandelbrotMapWorker());
    conf.setReducer(new MandelbrotReduceWorker());
    conf.setProjectName("mandelbrot");
		conf.launch();
	}
}

  /*
public class SETI_EPFL implements EntryPoint {
	public void onModuleLoad() {
	  JobConf<SPartialyAssignedExpression, SVoid, SVoid, SBoolAssign> conf = 
	      new JobConf<SPartialyAssignedExpression, SVoid, SVoid, SBoolAssign>();
	  conf.setMapper(new SatMapWorker());
	  conf.setReducer(new SatReduceWorker());
	  conf.setProjectName("sat");
	  conf.launch();
	}
}
*/