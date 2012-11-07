package ch.epfl.seti.client.custom;

import ch.epfl.seti.client.IMapWorker;
import ch.epfl.seti.client.IOutputCollector;
import ch.epfl.seti.client.typelib.SIntMatrix;
import ch.epfl.seti.client.typelib.SInteger;
import ch.epfl.seti.client.typelib.SVoid;
import ch.epfl.seti.shared.Pair;

import com.google.gwt.user.client.Window;


public class MandelbrotMapWorker implements
		IMapWorker<SInteger, SVoid, SVoid, SIntMatrix> {

  @Override
  public void map(SInteger key, SVoid value, IOutputCollector<Pair<SVoid, SIntMatrix>> output) {
    output.emit(new Pair<SVoid, SIntMatrix>(SVoid.UNIT, Mandelbrot.compute(key.i)));
  }
}

class Mandelbrot {
	public static int mand(Complex z0, int max) {
		Complex z = z0;
		for (int t = 0; t < max; t++) {
			if (z.abs() > 2.0)
				return t;
			z = z.times(z).plus(z0);
		}
		return max;
	}

	public static SIntMatrix compute(int offset) {
	  int N = 4096;
		double xc = 0;
		double yc = 0;
		double size = 2;

		int max = 255; // maximum number of iterations

		SIntMatrix pic = new SIntMatrix(4, N);

		long begin = System.currentTimeMillis();
		for (int i = offset; i < offset + 4; i++) {
			for (int j = 0; j < N; j++) {
				double x0 = xc - size / 2 + size * i / N;
				double y0 = yc - size / 2 + size * j / N;
				Complex z0 = new Complex(x0, y0);
				int gray = max - mand(z0, max);
				pic.data[i - offset][j] = gray;
			}
		}
		long end = System.currentTimeMillis();

//		Window.alert("Computation took " + ((end - begin) / 1000.0) + " seconds");
		return pic;
	}
}

class Complex {
	private final double re; // the real part
	private final double im; // the imaginary part

	// create a new object with the given real and imaginary parts
	public Complex(double real, double imag) {
		re = real;
		im = imag;
	}

	// return abs/modulus/magnitude and angle/phase/argument
	public double abs() {
		return Math.hypot(re, im);
	}

	// return a new Complex object whose value is (this + b)
	public Complex plus(Complex b) {
		Complex a = this; // invoking object
		double real = a.re + b.re;
		double imag = a.im + b.im;
		return new Complex(real, imag);
	}

	// return a new Complex object whose value is (this * b)
	public Complex times(Complex b) {
		Complex a = this;
		double real = a.re * b.re - a.im * b.im;
		double imag = a.re * b.im + a.im * b.re;
		return new Complex(real, imag);
	}
}
