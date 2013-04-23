import java.io.BufferedReader;
import java.io.InputStreamReader;
import java.net.URL;
import java.net.URLConnection;
import java.util.ArrayList;
import java.util.Date;
import java.util.Iterator;


public class MimeTypeGenerator {

	static class MimeType {
		public MimeType(String mimeType, String extension) {
			this.mimeType = mimeType;
			this.extension = extension;
		}

		public String mimeType;
		public String extension;
	}
	
	public static void main(String[] args) {
		BufferedReader in = null;
		ArrayList<MimeType> mimeTypes;
		try {
		URL website = new URL(args.length > 0 ? args[0] : "http://svn.apache.org/viewvc/httpd/httpd/trunk/docs/conf/mime.types?revision=1301894&view=co");

        URLConnection connection = website.openConnection();
        in = new BufferedReader(new InputStreamReader(connection.getInputStream()));

        StringBuilder response = new StringBuilder();
        String inputLine;

        mimeTypes = new ArrayList<MimeType>();

        while ((inputLine = in.readLine()) != null) {
        	String[] splits = inputLine.split(" |\t");
        	if (splits.length >= 1) {
        		if (!splits[0].contains("#")) {
        			String mimeType = splits[0].trim();
        			if (mimeType.length() > 0) {
	        			for(int i = 1; i < splits.length; i++) {
	        				String extension = splits[i].trim();
	        				if (extension.length() > 0) {
	        					mimeTypes.add(new MimeType(mimeType, extension));
	        				}
	        			}
	        		}
        		}
        	}
        }
		} catch (Exception e) {
			return;
		} finally {
			if (in != null) {
				try {
					in.close();
				} catch (Exception ee) {
				}
			}
		}
		
		Iterator<MimeType> iterator = mimeTypes.iterator();
		System.out.println("#pragma once\n");
		System.out.println("// generated at " + new Date());
		System.out.println("// BrowserSourcePlugin\n//");
		System.out.println("static CTSTR mimeTypes[] = {");
		while(iterator.hasNext()) {
			MimeType mimeType = iterator.next();
			System.out.print("    L\"" + mimeType.mimeType + "\", L\"" + mimeType.extension + "\"");
			if (iterator.hasNext()) {
				System.out.print(",");
			}
			System.out.println("");
		}
		System.out.println("};");
	}
}