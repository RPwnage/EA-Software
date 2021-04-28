/**
 *  HeatDecodeTest.java
 *
 *  (c) Electronic Arts. All Rights Reserved.
 *
 */
package ea.blaze.tests.component.arson;

import java.io.File;
import java.io.FileInputStream;
import java.io.FileOutputStream;
import java.io.InputStream;
import java.nio.ByteBuffer;
import java.nio.MappedByteBuffer;
import java.nio.channels.FileChannel;
import java.nio.channels.FileChannel.MapMode;
import java.security.MessageDigest;
import ea.blaze.event.tdf.EventEnvelope;
import ea.blaze.tdf.TdfCompare;
import ea.blaze.tdf.protocol.Heat2Decoder;
import ea.blaze.tdf.protocol.Heat2Encoder;
import ea.blaze.tdf.protocol.Heat2Util;
import ea.blaze.tdf.protocol.PrintEncoder;


/**
 * Simple test application to decode and encode HEAT2 encoded TDFs that's stored off into a file,
 * and test its idempotence via manual verification from println and MD5 hashing the re-encoded decoded data
 */
public class HeatDecodeTest {
	

	
	/**
	 * The main method.
	 * 
	 * @param args
	 *            the arguments
	 */
	public static void main(String[] args) {
		//testTdfDecoding();
		try {
			
			//The source binary file will need to be located relative to the java working folder 
			//(ie. \mainline\blazeserver\dev3\java\data\dev3)
			File sourceFolder = new File("data/dev3");				
			File[] sourceFileList = sourceFolder.listFiles();		
			
			for (File sourceEventFile:sourceFileList)
			{
				if (!sourceEventFile.isDirectory())	//only perform the work on non-directory
				{
				
					String basename = sourceEventFile.getName();
					String folder = sourceEventFile.getParent();
					
					System.out.println(basename + " " + folder);
					
					//1.	Pass in a HEAT2 encoded Blaze log file 
					//2.	Decode contents and print its contents out
					//3.	*Manual Verification* Compare textual content output with the heat2logreader utility (written by William Tan)
					//4.	Encode back to HEAT2 the decoded content, and save the output to binary log file
					System.out.println("Decoding the binary encoded log, and re-encode the decoded TDF to binary log file...");				
					String sourceFileName = sourceEventFile.getPath();			
					for (int i = 1; i <= 2; ++i)
					{
						System.out.println("Round " + Integer.toString(i));
						String decodeFileName = folder + "/output/" + basename + ".java.text."+ Integer.toString(i) +".log";
						String encodeFileName = folder + "/output/" + basename + ".java.binary."+ Integer.toString(i) +".log";
						decodeAndReencodeServerLog(sourceFileName, decodeFileName, encodeFileName);
						
						//check the encoded binary file with the source file.
						String md5JavaDecodedFilename = getMD5Checksum(sourceFileName);
						String md5JavaDerviedDecodeFilename = getMD5Checksum(encodeFileName);
				
						System.out.println("\nDecoded Content Checksum: " + md5JavaDecodedFilename);
						System.out.println("Dervied Decoded Content Checksum: " + md5JavaDerviedDecodeFilename);
						
						if (md5JavaDecodedFilename.equals(md5JavaDerviedDecodeFilename))
						{
							System.out.println("MD5 checksum verified.");				
						}
						else
						{
							System.out.println("MD5 checksum for (" + sourceFileName + ") and java derived decoded ("+encodeFileName+")are NOT the same!");
							throw new Exception("MD5 checksum for the java decoded and java derived decoded are NOT the same!");
						}
						sourceFileName = encodeFileName;			
					}
				}
			}
						
		}
		catch(Exception ex) {
			System.err.println("Exception thrown during log parsing: " + ex.getMessage());
			ex.printStackTrace(System.err);
		}
	}
	
	public static void decodeAndReencodeServerLog(String inFilename, String outDecodedFilename, String outEncodedFilename) throws Exception {
		File inLogFile = new File(inFilename);
		
		boolean bReEncode = false;
		File outEncodedLogFile = null;
		FileChannel outEncodedFileChannel = null;
		Heat2Encoder encoder = null;
		
		boolean bDecodeFileLogging = false;
		File outDecodedLogFile = null;
		FileOutputStream decodeFileOutputStream = null;
		
		//if decoder file logging is desired
		if (!outDecodedFilename.isEmpty())
		{
			outDecodedLogFile = new File(outDecodedFilename);
			
			System.out.println("Create the output folder if it doesn't exist");
			outDecodedLogFile.getParentFile().mkdir(); 
						
			decodeFileOutputStream = new FileOutputStream(outDecodedLogFile);
			
			bDecodeFileLogging = true;
		}
		
		//if re-encoding is desired
		if (!outEncodedFilename.isEmpty())
		{
			outEncodedLogFile = new File(outEncodedFilename);
			outEncodedFileChannel = (new FileOutputStream(outEncodedLogFile)).getChannel();	
					
			encoder = new Heat2Encoder();
			
			bReEncode = true;
		}
		
		if(inLogFile.exists()) {
			FileChannel inFileChannel = (new FileInputStream(inLogFile)).getChannel();
			
			if(inFileChannel.size() > 0) {
				Heat2Decoder decoder = new Heat2Decoder(true, true);
				EventEnvelope logEventTDF = new EventEnvelope();
				
				ByteBuffer eventHeader = ByteBuffer.allocate(8);
				
				while(inFileChannel.position() < inFileChannel.size()) {
					
					eventHeader.clear();
					
					inFileChannel.read(eventHeader);
					eventHeader.rewind();
					
					long sequenceNumber = Heat2Util.promoteUnsignedInteger(eventHeader.getInt());
					long dataSize = Heat2Util.promoteUnsignedInteger(eventHeader.getInt());

					if (dataSize > Integer.MAX_VALUE || dataSize < 0)
					{
						System.out.println(dataSize);
						System.out.println(eventHeader);
						System.out.println(inLogFile);
						throw new Exception();
					}
					
					ByteBuffer tdfBuffer = ByteBuffer.allocate((int)dataSize);
					
					inFileChannel.read(tdfBuffer);
					tdfBuffer.rewind();
					
					//2.	Decode contents 
					decoder.decode(tdfBuffer, logEventTDF);
					tdfBuffer.clear();
					
					if(decoder.decodeResult()) {
						System.out.println("Successfully read log [" + inLogFile.getName() + "] sequence number " + sequenceNumber + ":");
						
						//2. print decoded tdf content out to screen
						PrintEncoder.printTdf(System.out, logEventTDF);
						if (bDecodeFileLogging)  //print to output file
						{							
							PrintEncoder.printTdf(decodeFileOutputStream, logEventTDF);
						}
						
						//4.	Encode back to HEAT2 the decoded content, and 
						//		save the output to binary log file
						if (bReEncode)
						{
							//write the event header as-is
							eventHeader.rewind();						
							outEncodedFileChannel.write(eventHeader);
							
							//encode and write the encoded event content
							//buffer is allocated internally in encoder.encode()
							ByteBuffer tdfOutBuffer = encoder.encode(logEventTDF);
							tdfOutBuffer.flip();
							outEncodedFileChannel.write(tdfOutBuffer);
						}
					}
				}
				
				if (bReEncode)
				{
					outEncodedFileChannel.close();
				}
				
				if (bDecodeFileLogging)  //print to output file
				{
					decodeFileOutputStream.close();
				}
			}
			else {
				System.out.println("Log file [" + inLogFile.getName() + "] has no data available.");
			}
		}
		else {
			System.err.println("Specified log file [" + inFilename + "] does not exist.");
		}
	}
	
	/**
	 * Test tdf decoding.
	 */
	public static void testTdfDecoding() {
		try {	
			FileChannel fileChannel = (new FileInputStream("C:\\tdfEncode.dat")).getChannel();

			if(fileChannel == null) {
				System.err.println("Failed to open tdfEncode.dat");
				return;
			}
	
			MappedByteBuffer mappedByteBuffer = fileChannel.map(MapMode.READ_ONLY, 0, fileChannel.size());
			
			Heat2Decoder decoder = new Heat2Decoder(true, true);
			
			ea.blaze.arson.tdf.MyTest testTdf = new ea.blaze.arson.tdf.MyTest();
			
			decoder.decode(mappedByteBuffer, testTdf);
			
			if(decoder.decodeResult()) {
				System.out.println("Decoding succeeded!");

				ea.blaze.arson.tdf.MyTest copiedTdf = new ea.blaze.arson.tdf.MyTest(testTdf);
				
				if(TdfCompare.compareTdfs(testTdf, copiedTdf)) {
					System.out.println("Copy constructor seems to work.");
					PrintEncoder.printTdf(System.out, copiedTdf);
				}
				else {
					System.err.println("Copy constructor does not seem to work.");
				}
				
				Heat2Encoder encoder = new Heat2Encoder();
				ByteBuffer outputBuffer = ByteBuffer.allocate((int)fileChannel.size());
				
				encoder.encode(outputBuffer, testTdf);
				
				if(encoder.encodeResult()) {
					System.out.println("Encoding succeeded!");
					outputBuffer.rewind();
					FileOutputStream outputStream = new FileOutputStream("C:\\tdfEncodeRoundTrip.dat");
					outputStream.getChannel().write(outputBuffer);
				}
				else {
					System.err.println("Encoding failed.");
				}
			}
			else {
				System.err.println("Decoding failed.");
			}
			
		}
		catch(Exception ex) {
			System.err.println("Exception thrown while decoding TDF: " + ex.getMessage());
			ex.printStackTrace(System.err);
		}
		
	}
	
///////////////////////////////////////////////////////////////////////////////////	
// Method to return the MD5 digest of a file
// Source: http://www.rgagnon.com/javadetails/java-0416.html
public static byte[] createChecksum(String filename) throws Exception
{
	InputStream fis =  new FileInputStream(filename);

	byte[] buffer = new byte[1024];
	MessageDigest complete = MessageDigest.getInstance("MD5");
	int numRead;
	do {
		numRead = fis.read(buffer);
		if (numRead > 0) 
		{
			complete.update(buffer, 0, numRead);
		}
	} while (numRead != -1);
	fis.close();
	return complete.digest();
}

// a byte array to a HEX string
public static String getMD5Checksum(String filename) throws Exception 
{
	byte[] b = createChecksum(filename);
	String result = "";
	for (int i=0; i < b.length; i++) 
	{
		result += Integer.toString( ( b[i] & 0xff ) + 0x100, 16).substring( 1 );
	}
	return result;
}

///////////////////////////////////////////////////////////////////////////////////	

}
