/**
 *  HttpTest.java
 *
 *  (c) Electronic Arts. All Rights Reserved.
 *
 */
package ea.blaze.tests.component.arson;

import java.io.BufferedReader;
import java.io.File;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.InputStreamReader;
import java.net.HttpURLConnection;
import java.net.URL;
import java.nio.ByteBuffer;

import ea.blaze.arson.tdf.MyTest;
import ea.blaze.tdf.ComponentMetricsRequest;
import ea.blaze.tdf.ComponentMetricsResponse;
import ea.blaze.tdf.ServerStatus;
import ea.blaze.tdf.Tdf;
import ea.blaze.tdf.TdfCompare;
import ea.blaze.tdf.protocol.HttpEncoder;
import ea.blaze.tdf.protocol.PrintEncoder;
import ea.blaze.tdf.protocol.XmlDecoder;
import ea.blaze.tdf.types.TdfInteger;
import ea.blaze.tdf.types.TdfString;

/**
 * Simple test application to test HTTP encoder and XML decoder using the web access layer of a 
 * blazeserver running on localhost with arson component enabled. *
 */
public class HttpTest {

	/**
	 * Make rpc call.
	 * 
	 * @param rpcComponent
	 *            the rpc component
	 * @param rpcMethod
	 *            the rpc method
	 * @param tdfIn
	 *            the tdf in
	 * @param tdfOut
	 *            the tdf out
	 * @return true, if successful
	 */
	public static boolean makeRpcCall(String rpcComponent, String rpcMethod, Tdf tdfIn, Tdf tdfOut) {
		HttpEncoder httpEncoder = new HttpEncoder();
		XmlDecoder xmlDecoder = new XmlDecoder();
		
		try {
			String tdfRequestString = "";
			
			if(tdfIn != null) {
				ByteBuffer paramBuffer = ByteBuffer.allocate(4096);
				httpEncoder.encode(paramBuffer, tdfIn);
		    	tdfRequestString = "?" + new String(paramBuffer.asCharBuffer().toString());
			}
	    	
	    	URL rpcRequestURL = new URL("http://localhost:43728/" + rpcComponent + "/" + rpcMethod + tdfRequestString);  	
	    	System.out.println("Request URL: " + rpcRequestURL.toString());
	    	
	    	HttpURLConnection urlConnection = (HttpURLConnection)rpcRequestURL.openConnection();
	    	urlConnection.setReadTimeout(15*1000);
	    	urlConnection.connect();    	
	    	
	    	BufferedReader reader = new BufferedReader(new InputStreamReader(urlConnection.getInputStream()));
	    	StringBuilder builder = new StringBuilder();
	    	String line = null;
	    	
	    	while((line = reader.readLine()) != null) {
	    		builder.append(line + "\n");
	    	}
	    	
	    	System.out.println("Returned XML:");
	    	System.out.println(builder.toString());

	    	ByteBuffer xmlInputBuffer = ByteBuffer.allocate(builder.toString().getBytes("UTF-8").length);
	    	xmlInputBuffer.put(builder.toString().getBytes("UTF-8"));
	    	xmlInputBuffer.rewind();
	    	xmlDecoder.decode(xmlInputBuffer.asReadOnlyBuffer(), tdfOut);
	    	return xmlDecoder.decodeResult();
		}
		catch(Exception ex) {
			System.err.println("Exception thrown when attempting to make echo request: " + ex.getMessage());
			ex.printStackTrace(System.err);
		}
		
		return false;
	}
	
	/**
	 * The main method.
	 * 
	 * @param args
	 *            the arguments
	 */
	public static void main(String[] args) {

		MyTest mt = new MyTest();

		mt.setInt(42);
	    mt.setBar(new ea.blaze.arson.tdf.Bar());
	    mt.getBar().setBarInt(99);
	    mt.getBar().setBarFoo(new ea.blaze.arson.tdf.Foo());
	    mt.getBar().getBarFoo().setFooInt(21);
	    mt.setFoo(new ea.blaze.arson.tdf.Foo());
	    mt.getFoo().setFooInt(98);
	    mt.setInt2(20);
	    mt.getIntList().add(new TdfInteger(1));
	    mt.getIntList().add(new TdfInteger(2));
	    mt.getIntList().add(new TdfInteger(3));
	    mt.setInt3(40);
	    mt.getStringList().add(new TdfString("Hello"));
	    mt.getStringList().add(new TdfString("World"));
	    mt.setInt4(60);
	    ea.blaze.arson.tdf.Bar bar = new ea.blaze.arson.tdf.Bar();
	    bar.getBarFoo().setFooInt(21);
	    bar.setBarInt(12);
	    ea.blaze.arson.tdf.Bar bar2 = new ea.blaze.arson.tdf.Bar();
	    bar2.getBarFoo().setFooInt(13);
	    bar2.setBarInt(69);
	    mt.getBarMap().put(new TdfString("Alan"), bar);
	    mt.getBarMap().put(new TdfString("Alan2"), bar2);
	    mt.getBarMap().put(new TdfString("Alan3"), new ea.blaze.arson.tdf.Bar());
	    mt.setInt5(80);
	    mt.setInt6(100);
	    mt.getBarMapIntInt().put(new TdfInteger(55), new TdfInteger(1));
	    mt.getBarMapIntInt().put(new TdfInteger(97), new TdfInteger(2));
	    mt.getBarMapIntInt().put(new TdfInteger(99), new TdfInteger(3));
	    mt.setInt7(120);
	    mt.setInt8(140);
	    ea.blaze.arson.tdf.MyUnion union = new ea.blaze.arson.tdf.MyUnion();
	    union.setInt(81L);
	    mt.setUnion1(union);
	    mt.setInt9(new Integer(160));
	    ea.blaze.arson.tdf.MyUnion union2 = new ea.blaze.arson.tdf.MyUnion();
	    union2.setString(new String("Test"));
	    mt.setUnion2(union2);
	    mt.setInt10(new Integer(180));
	    ea.blaze.arson.tdf.MyUnion union3 = new ea.blaze.arson.tdf.MyUnion();
	    ea.blaze.arson.tdf.Foo unionFoo = new ea.blaze.arson.tdf.Foo();
	    unionFoo.setFooInt(1975);
	    union3.setFoo(unionFoo);
	    mt.setUnion3(union3);
	    
	    mt.setInt11(new Integer(200));
	    ea.blaze.arson.tdf.MyUnion union4 = new ea.blaze.arson.tdf.MyUnion();
	    ea.blaze.arson.tdf.Bar unionBar = new ea.blaze.arson.tdf.Bar();
	    unionBar.setBarInt(1997);
	    unionBar.setBarFoo(unionFoo);
	    union4.setBar(unionBar);
	    
	    mt.setUnion4(union4);
	    mt.setString3(new String("string3"));
	    mt.setString2(new String("string2"));
	    mt.setString(new String("string"));
	    
	    /*
	    ea.blaze.tdf.Blob blob = new ea.blaze.tdf.Blob();
	    blob.setData(new byte[] { 0x0, 0x1, 0x2, 0x3, 0x4, 0x5 });
	    
	    System.out.println("Blob data size is: " + blob.getData().length);
	    
	    Base64Encoder encoder = new Base64Encoder();
	    String blobString = encoder.encode(blob.getData());
	    System.out.println("Encoded blob data is: \"" + blobString + "\"");
	    byte blobArray[] = encoder.decode(blobString.getBytes());
	    blobString = encoder.encode(blobArray);
	    System.out.println("Round trip blob data is: \"" + blobString + "\"");
	    
	    mt.setBlob(blob);
	    */
	    
	   // mt.setTestVariable(new ea.blaze.tdf.VariableTdfContainer());
	    ea.blaze.arson.tdf.Foo foo = new ea.blaze.arson.tdf.Foo();
	    foo.setFooInt(42);
	    
	   // mt.getTestVariable().set(bar);
	    
	    MyTest mtReturned = new MyTest();
	    
	    if(makeRpcCall("arson", "echo", mt, mtReturned)) {
	    	System.out.println("RPC request succeeded, comparing result.");
	    	TdfCompare compareTdfs = new TdfCompare();
	    	if(compareTdfs.equals(mt, mtReturned)) {
	    		System.out.println("Returned TDF is identical to original TDF.");
	    		MyTest mtThirdTime = new MyTest();
	    		if(makeRpcCall("arson", "echo", mtReturned, mtThirdTime)) {
	    	    	System.out.println("RPC request succeeded, comparing result.");
	    	    	if(compareTdfs.equals(mtThirdTime, mtReturned)) {
	    	    		System.out.println("Returned TDF is identical to original TDF.");
	    	    	}
	    	    	else {
	    	    		System.out.println("ERROR: Returned TDF is different to the original TDF.");
	    	    	}
	    		}
	    	}
	    	else {
	    		System.out.println("ERROR: Returned TDF is different to the original TDF.");
	    		try {
		    		FileOutputStream fout = new FileOutputStream(new File("C:\\originalTdf.txt"));
		    		PrintEncoder.printTdf(fout, mt);
		    		fout = new FileOutputStream(new File("C:\\returnedTdf.txt"));
		    		PrintEncoder.printTdf(fout, mtReturned);
	    		}
	    		catch(Exception ex) {
	    			System.err.println("Failed to write TDFs to output files: " + ex.getMessage());
	    			ex.printStackTrace(System.err);
	    		}
	    	}

	    }
	   	    
	    ServerStatus status = new ServerStatus();
	    if(makeRpcCall("blazecontroller", "getStatus", null, status)) {
	    	System.out.println("Got server status.");
	    	try {
	    		PrintEncoder.printTdf(System.out, status);
	    	}
	    	catch(IOException ex) {
	    		System.err.println("Failed to print TDF to System.out: " + ex.getMessage());
	    		ex.printStackTrace(System.err);
	    	}
	    }
	    else {
	    	System.err.println("Failed to get server status.");
	    }
	    
	    ComponentMetricsResponse metrics = new ComponentMetricsResponse();
	    
	    if(makeRpcCall("blazecontroller", "getComponentMetrics", new ComponentMetricsRequest(), metrics)) {
	    	System.out.println("Got metrics response, writing to C:\\metricsResponse.txt");
	    	try {
		    	FileOutputStream fout = new FileOutputStream(new File("C:\\metricsResponse.txt"));
		    	PrintEncoder.printTdf(fout, metrics);
	    	}
	    	catch(IOException ex) {
	    		System.err.println("IOException while writing out metrics: " + ex.getMessage());
	    		ex.printStackTrace(System.err);
	    	}
	    }
	    else {
	    	System.err.println("Failed to get component metrics.");
	    }
	}

}

