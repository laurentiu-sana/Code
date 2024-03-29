package ch.epfl.seti.server.custom;

import java.awt.Color;
import java.awt.FileDialog;
import java.awt.Toolkit;
import java.awt.event.ActionEvent;
import java.awt.event.ActionListener;
import java.awt.event.KeyEvent;
import java.awt.image.BufferedImage;
import java.io.File;
import java.io.IOException;

import javax.imageio.ImageIO;
import javax.swing.ImageIcon;
import javax.swing.JFrame;
import javax.swing.JLabel;
import javax.swing.JMenu;
import javax.swing.JMenuBar;
import javax.swing.JMenuItem;
import javax.swing.KeyStroke;

public final class MandelbrotPicture implements ActionListener {
  private BufferedImage image; // the rasterized image
  private JFrame frame; // on-screen view
  private String filename; // name of file
  private boolean isOriginUpperLeft = true; // location of origin
  private int width, height; // width and height

  /**
   * Create a blank w-by-h picture, where each pixel is black.
   */
  public MandelbrotPicture(int w, int h) {
    width = w;
    height = h;
    image = new BufferedImage(w, h, BufferedImage.TYPE_INT_RGB);
    // set to TYPE_INT_ARGB to support transparency
    filename = w + "-by-" + h;
  }

  /**
   * Return a JLabel containing this Picture, for embedding in a JPanel, JFrame
   * or other GUI widget.
   */
  public JLabel getJLabel() {
    if (image == null) {
      return null;
    } // no image available
    ImageIcon icon = new ImageIcon(image);
    return new JLabel(icon);
  }

  /**
   * Set the origin to be the upper left pixel.
   */
  public void setOriginUpperLeft() {
    isOriginUpperLeft = true;
  }

  /**
   * Set the origin to be the lower left pixel.
   */
  public void setOriginLowerLeft() {
    isOriginUpperLeft = false;
  }

  /**
   * Display the picture in a window on the screen.
   */
  public void show() {

    // create the GUI for viewing the image if needed
    if (frame == null) {
      frame = new JFrame();

      JMenuBar menuBar = new JMenuBar();
      JMenu menu = new JMenu("File");
      menuBar.add(menu);
      JMenuItem menuItem1 = new JMenuItem(" Save...   ");
      menuItem1.addActionListener(this);
      menuItem1.setAccelerator(KeyStroke.getKeyStroke(KeyEvent.VK_S, Toolkit
          .getDefaultToolkit().getMenuShortcutKeyMask()));
      menu.add(menuItem1);
      frame.setJMenuBar(menuBar);

      frame.setContentPane(getJLabel());
      // f.setDefaultCloseOperation(JFrame.EXIT_ON_CLOSE);
      frame.setDefaultCloseOperation(JFrame.DISPOSE_ON_CLOSE);
      frame.setTitle(filename);
      frame.setResizable(false);
      frame.pack();
      frame.setVisible(true);
    }

    // draw
    frame.repaint();
  }

  /**
   * Return the height of the picture in pixels.
   */
  public int height() {
    return height;
  }

  /**
   * Return the width of the picture in pixels.
   */
  public int width() {
    return width;
  }

  /**
   * Return the color of pixel (i, j).
   */
  public Color get(int i, int j) {
    if (isOriginUpperLeft)
      return new Color(image.getRGB(i, j));
    else
      return new Color(image.getRGB(i, height - j - 1));
  }

  /**
   * Set the color of pixel (i, j) to c.
   */
  public void set(int i, int j, Color c) {
    if (c == null) {
      throw new RuntimeException("can't set Color to null");
    }
    if (isOriginUpperLeft)
      image.setRGB(i, j, c.getRGB());
    else
      image.setRGB(i, height - j - 1, c.getRGB());
  }

  /**
   * Save the picture to a file in a standard image format. The filetype must be
   * .png or .jpg.
   */
  public void save(String name) {
    save(new File(name));
  }

  /**
   * Save the picture to a file in a standard image format.
   */
  public void save(File file) {
    this.filename = file.getName();
    System.err.println("Saved to " + file.getAbsolutePath());
    if (frame != null) {
      frame.setTitle(filename);
    }

    String suffix = filename.substring(filename.lastIndexOf('.') + 1);
    suffix = suffix.toLowerCase();
    if (suffix.equals("jpg") || suffix.equals("png")) {
      try {
        ImageIO.write(image, suffix, file);
      } catch (IOException e) {
        e.printStackTrace();
      }
    } else {
      System.out.println("Error: filename must end in .jpg or .png");
    }
  }

  /**
   * Opens a save dialog box when the user selects "Save As" from the menu.
   */
  public void actionPerformed(ActionEvent e) {
    FileDialog chooser = new FileDialog(frame, "Use a .png or .jpg extension",
        FileDialog.SAVE);
    chooser.setVisible(true);
    if (chooser.getFile() != null) {
      save(chooser.getDirectory() + File.separator + chooser.getFile());
    }
  }
}
