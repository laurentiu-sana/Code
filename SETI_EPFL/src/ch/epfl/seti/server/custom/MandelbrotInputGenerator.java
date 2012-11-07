package ch.epfl.seti.server.custom;

import java.util.concurrent.atomic.AtomicInteger;

import ch.epfl.seti.client.typelib.SInteger;
import ch.epfl.seti.client.typelib.SVoid;
import ch.epfl.seti.server.IInputGenerator;
import ch.epfl.seti.shared.MapTask;
import ch.epfl.seti.shared.Pair;
import ch.epfl.seti.shared.Task;

public class MandelbrotInputGenerator implements IInputGenerator<SInteger, SVoid>{
  private AtomicInteger m_counter = new AtomicInteger(0);
  int N = 4096;
  @Override
  public boolean hasNextTask() {
    return m_counter.get() < N;
  }

  @Override
  public Task<SInteger, SVoid> nextTask() {
    int offset = m_counter.getAndAdd(4);
    if (offset >= N) {
      m_counter.getAndAdd(-4);
      return null;
    }

    MapTask<SInteger,SVoid> ret = new MapTask<SInteger, SVoid>();
    ret.add(new Pair<SInteger, SVoid>(new SInteger(offset), SVoid.UNIT));
    return ret;
  }

  @Override
  public boolean executeReduceOnServer() {
    return true;
  }
}
