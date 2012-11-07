package ch.epfl.seti.client.custom;

import ch.epfl.seti.client.IOutputCollector;
import ch.epfl.seti.client.IReduceWorker;
import ch.epfl.seti.client.typelib.SIntMatrix;
import ch.epfl.seti.client.typelib.SVector;
import ch.epfl.seti.client.typelib.SVoid;

public class MandelbrotReduceWorker implements IReduceWorker<SVoid, SIntMatrix> {

  @Override
  public void reduce(SVoid key, SVector<SIntMatrix> value,
      IOutputCollector<SIntMatrix> output) {
    int N = 4096;
    SIntMatrix pic = new SIntMatrix(N, N);
    int offset = 0;
    
    for (SIntMatrix data: value) {
      boolean invertGray = (offset / 4) % 2 == 0;
      
      for (int i = 0; i < 4; i++) {
        for (int j = 0; j < N; j++) {
          int gray = data.data[i][j];
          if (invertGray) {
            gray = 255 - gray;
          }
          pic.data[i + offset][j] = gray;
        }
      }
      
      offset += 4;
    }

    output.emit(pic);
  }

}
