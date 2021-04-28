#include "services/debug/DebugService.h"
#include "GzipCompress.h"

namespace
{
    const qint64 OutputBufferGrowSize = 65536;
    const qint64 FileChunkSize = 65536; 

    const void initGzipStream(z_stream &stream)
    {
	    stream.zalloc = Z_NULL;
        stream.zfree = Z_NULL;
        stream.opaque = Z_NULL;

	    // Create a gzip stream
	    // The magic number of 16 triggers gzip mode - apparently there's no constant for this
	    if (deflateInit2(&stream, 9, Z_DEFLATED, 16 + 15, 8, Z_DEFAULT_STRATEGY) != Z_OK)
	    {
		    ORIGIN_ASSERT(0);
	    }
    }
}

namespace Origin
{
    namespace Services
    {
        QByteArray gzipData(const char *data, int length)
        {
	        z_stream stream;
	        initGzipStream(stream);

	        // The input is fixed
	        stream.avail_in = length;
	        stream.next_in = (Bytef*)data;

	        // Start assuming the output is the same length as the input
	        QByteArray output;
	        output.resize(length);

	        stream.avail_out = output.size();
	        stream.next_out = (Bytef*)output.data();

	        while(true) 
	        {
		        int result = deflate(&stream, Z_FINISH);

		        if (result == Z_STREAM_END)
		        {
			        // All done!
			        // Truncate the part of the buffer we didn't use
			        output.truncate(output.size() - stream.avail_out);
			        break;
		        }
		
		        ORIGIN_ASSERT((result == Z_OK) || (result == Z_BUF_ERROR && length == 0));
		        ORIGIN_ASSERT(stream.avail_out == 0);

		        // Create more buffer room
		        int oldSize = output.size();
		        output.resize(oldSize + OutputBufferGrowSize);

		        stream.avail_out = OutputBufferGrowSize;
		        stream.next_out = (Bytef*)(output.data() + oldSize);
	        }

	        deflateEnd(&stream);

	        return output;
        }

        QByteArray gzipData(const QByteArray &data)
        {
	        return gzipData(data.constData(), data.size());
        }

        bool gzipFile(QFile &file, QFile *zipFile)
        {
	        qint64 size = file.size();
            qint64 offset = 0;
            uchar *data = NULL;
            qint64 length;
            QByteArray output;
            int result, flush;

            z_stream stream;
	        initGzipStream(stream);

            // Loop through the file, compressing FileChunkSize - or less - bytes on each loop, and writing them to the zipped file
            do
            {
	            qint64 remainingSize = size - offset;
                if (remainingSize <= FileChunkSize)
                {
                    data = file.map(offset, remainingSize);
                    offset += remainingSize;
                    length = remainingSize;
                    flush = Z_FINISH;
                }
                else
                {
                    data = file.map(offset, FileChunkSize);
                    offset += FileChunkSize;
                    length = FileChunkSize;
                    flush = Z_NO_FLUSH;
                }

                // map() returns zero if it fails
                if (!data)
                {
                    deflateEnd(&stream);
                    zipFile->close();
                    return false;
                }

	            stream.avail_in = length;
	            stream.next_in = (Bytef*)data;
                output.resize(FileChunkSize);

	            // Run deflate() on data until the output buffer is not being completely filled
                do {
                    stream.avail_out = FileChunkSize;
	                stream.next_out = (Bytef*)output.data();

		            result = deflate(&stream, flush);
                    ORIGIN_ASSERT(result != Z_STREAM_ERROR);

			        output.truncate(output.size() - stream.avail_out);
		            if (zipFile->write(output) == -1)
                    {
                        // write() failed
                        deflateEnd(&stream);
                        zipFile->close();
                        return false;
                    }
                } while (stream.avail_out == 0);

                file.unmap(data);  
            } while (flush != Z_FINISH);
            ORIGIN_ASSERT(result == Z_STREAM_END);

            deflateEnd(&stream);
            zipFile->close();

            return true;
        }
    }
}

