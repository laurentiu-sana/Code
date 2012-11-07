package ch.epfl.seti.client;

import java.util.Iterator;

import ch.epfl.seti.client.typelib.SVector;
import ch.epfl.seti.shared.Constants;
import ch.epfl.seti.shared.MapTask;
import ch.epfl.seti.shared.NoTaskAvailableException;
import ch.epfl.seti.shared.Pair;
import ch.epfl.seti.shared.ReduceTask;
import ch.epfl.seti.shared.Task;

import com.google.gwt.core.client.GWT;
import com.google.gwt.user.client.Timer;
import com.google.gwt.user.client.Window;
import com.google.gwt.user.client.rpc.AsyncCallback;
import com.google.gwt.user.client.rpc.IsSerializable;

public class JobConf<K1 extends IsSerializable, V1 extends IsSerializable, K2 extends IsSerializable, V2 extends IsSerializable> {
  public final ServerServiceAsync<K1, V1, K2, V2> serverService = GWT
      .create(ServerService.class);
  private IMapWorker<K1, V1, K2, V2> m_mapper = null;
  private IReduceWorker<K2, V2> m_reducer = null;
  private String m_projectName = null;
  private int m_tasksCounter = 0;

  public void setMapper(IMapWorker<K1, V1, K2, V2> mapper) {
    m_mapper = mapper;
  }

  public void setReducer(IReduceWorker<K2, V2> reducer) {
    m_reducer = reducer;
  }

  public void setProjectName(String name) {
    m_projectName = name;
  }

  /** A client processes tasks until a null one is returned */
  @SuppressWarnings("rawtypes")
  public void launch() {
    AsyncCallback<Task> cb = new AsyncCallback<Task>() {
      /**
       * If the server did not give any task, retry after a delay 
       */
      @Override
      public void onFailure(Throwable caught) {
        if (caught instanceof NoTaskAvailableException) {
          Timer t = new Timer() {
            @Override
            public void run() {
              JobConf.this.launch();
            }
          };
          t.schedule(Constants.DELAY);
        }
      }

      @SuppressWarnings("unchecked")
      @Override
      public void onSuccess(Task result) {
        if (result != null) {
          if (result instanceof MapTask) {
            // Map task
            final MapOutputCollector oc = new MapOutputCollector(result.getId());

            Iterator<Pair<K1, V1>> it = ((MapTask<K1, V1>) result).iterator();

            /* TODO: here we should yield the processor from time to time */
            while (it.hasNext()) {
              Pair<K1, V1> workunit = it.next();
              m_mapper.map(workunit.getFirst(), workunit.getSecond(), oc);
            }

            if (Constants.DEBUG_TASK_TIMEOUT) {
              /** Upload the result after some time */
              Timer timer = new Timer() {
                public void run() {
                  oc.upload();
                }
              };
              timer.schedule(10 * Constants.TASK_TIMEOUT);
            } else {
              /** Upload the result */
              oc.upload();
            }
          } else if (result instanceof ReduceTask) {
            // Reduce task
            final ReduceOutputCollector oc = new ReduceOutputCollector(
                result.getId());
            ReduceTask<K2, V2> reduceTask = (ReduceTask<K2, V2>) result;

            m_reducer.reduce(reduceTask.getFirst(), reduceTask.getSecond(), oc);

            oc.upload();
          }

          if (++m_tasksCounter < Constants.TASKS_PER_CLIENT)
            launch();
        }
      }
    };

    serverService.getTask(m_projectName, cb);
  }

  /**
   * Internal class for collecting map results
   */
  private class MapOutputCollector implements IOutputCollector<Pair<K2, V2>> {
    private SVector<Pair<K2, V2>> m_data = new SVector<Pair<K2, V2>>();
    private int m_taskId;

    public MapOutputCollector(int taskId) {
      m_taskId = taskId;
    }

    public void emit(Pair<K2, V2> data) {
      m_data.add(data);
    }

    public void upload() {
      AsyncCallback<Void> ac = new AsyncCallback<Void>() {
        @Override
        public void onFailure(Throwable caught) {
        }

        @Override
        public void onSuccess(Void result) {
//          Window.alert("[Map] result uploaded!!");
        }
      };

      serverService.putMapResult(m_data, m_taskId, ac);
      m_data.clear();
    }
  }

  /**
   * Internal class for collecting reduce results
   */
  private class ReduceOutputCollector implements IOutputCollector<V2> {
    private SVector<V2> m_data;
    private int m_taskId;

    public ReduceOutputCollector(int taskId) {
      m_data = new SVector<V2>();
      m_taskId = taskId;
    }

    public void emit(V2 data) {
      m_data.add(data);
    }

    public void upload() {
      AsyncCallback<Void> ac = new AsyncCallback<Void>() {
        @Override
        public void onFailure(Throwable caught) {
        }

        @Override
        public void onSuccess(Void result) {
//          Window.alert("[Reduce] result uploaded!!");
        }
      };

      serverService.putReduceResult(m_data, m_taskId, ac);
      m_data.clear();
    }
  }
}
