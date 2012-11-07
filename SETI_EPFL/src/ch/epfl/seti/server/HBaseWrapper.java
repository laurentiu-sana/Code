package ch.epfl.seti.server;
import java.io.IOException;
import java.util.NavigableMap;

import org.apache.hadoop.conf.Configuration;
import org.apache.hadoop.hbase.HBaseConfiguration;
import org.apache.hadoop.hbase.client.Get;
import org.apache.hadoop.hbase.client.HTable;
import org.apache.hadoop.hbase.client.Put;
import org.apache.hadoop.hbase.client.Result;
import org.apache.hadoop.hbase.client.ResultScanner;
import org.apache.hadoop.hbase.client.Scan;
import org.apache.hadoop.hbase.util.Bytes;

public class HBaseWrapper {
	private final static String HBASE_TABLE_NAME = "seti";
	private final static String HBASE_FAMILY_NAME = "task";

	private static HBaseWrapper s_instance = null;
	private HTable m_htable = null;

	public void putData(String projectName, byte[] key, byte[] value) {
		try {
			Put put = new Put(Bytes.toBytes(projectName));
			put.add(Bytes.toBytes(HBASE_FAMILY_NAME), key, value);
			m_htable.put(put);
		} catch (Exception e) {
			e.printStackTrace();
		}
	}

	public void putData(String projectName, String key, String value) {
		putData(projectName, Bytes.toBytes(key), Bytes.toBytes(value));
	}

	public byte[] getData(String projectName, byte[] key) {
		try {
			Get get = new Get(Bytes.toBytes(projectName));
			Result r = m_htable.get(get);
			return r.getValue(Bytes.toBytes(HBASE_FAMILY_NAME), key);
		} catch (Exception e) {
			e.printStackTrace();
			return null;
		}
	}

	public String getData(String projectName, String key) {
		return new String(getData(projectName, Bytes.toBytes(key)));
	}

	public void debugProject(String projectName) {
		try {
			Scan scan = new Scan(Bytes.toBytes(projectName));
			ResultScanner scanner = m_htable.getScanner(scan);
			for (Result result : scanner) {
				NavigableMap<byte[], NavigableMap<byte[], NavigableMap<Long, byte[]>>> map = result.getMap();
				for (byte[] it: map.keySet()) {
					System.err.println("Column " + new String(it));
					NavigableMap<byte[], NavigableMap<Long, byte[]>> data = map.get(it);
					for (byte[] key: data.keySet()) {
						NavigableMap<Long, byte[]> values = data.get(key);
						for (Long timestamp: values.keySet()) {
							byte[] value = values.get(timestamp);
							System.err.println("  [" + timestamp + "] " + new String(key) + " " + new String(value));
						}
					}
				}
			}
		} catch (IOException e) {
			e.printStackTrace();
		}

	}

	public static HBaseWrapper createHBaseConnection() {
		if (s_instance == null) {
			s_instance = new HBaseWrapper();
			Configuration config = HBaseConfiguration.create();
			config.set("hbase.zookeeper.quorum", "localhost");
			try {
				s_instance.m_htable = new HTable(HBASE_TABLE_NAME);
			} catch (IOException e) {
				e.printStackTrace();
				return null;
			}
		}
		return s_instance;
	}

  public HTable getHTable() {
    return m_htable;
  }
}
