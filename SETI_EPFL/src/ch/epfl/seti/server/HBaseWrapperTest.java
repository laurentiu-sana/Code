package ch.epfl.seti.server;
public class HBaseWrapperTest {

	public static void main(String[] args) {
		String projectName = "sat";
		HBaseWrapper wrapper = HBaseWrapper.createHBaseConnection();
		wrapper.debugProject(projectName);
	}
}
