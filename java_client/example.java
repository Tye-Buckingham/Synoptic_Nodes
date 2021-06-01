import java.io.BufferedReader;
import java.io.InputStream;
import java.io.InputStreamReader;
import java.net.HttpURLConnection;
import java.net.URL;
import java.nio.charset.StandardCharsets;
import java.time.Instant;
import java.util.Base64;

/**
 *  Base HTTP request system for the mesh network
 *  Depending on the user selections a different url will be given to the server
 *
 *  Authentication has been added, only a password is needed if the username is permanently stored.
 *  All requests are current formatted strings, these can be parsed into a JSON or object within Java
 *  FORMATS:: *ignore []*
 *      /request will return a ticket number from 0-255 e.g. [5]
 *      /nodes will return a list of nodes - ID:[node_id],Name:[node_name]
 *      /check will return all current tickets - [ticket_num]:[ticket_num]
 *      /get-request will return [ticket_num]:[timestamp],[sensor_reading1],[sensor_reading2],...
 *      /time will broadcast and sync the time to the given unix timestamp
 *          - use when connecting or at intervals to keep the nodes in sync
 *
 *      Note: the user will use request to make their request and get a ticket number
 *      then use get-request to retrieve their request when it is ready
 *      once the request is received the ticket is erased
 */
public class Main {

    private static String nodes_list = "http://192.168.1.103/nodes";


    private static String check = "http://192.168.1.103/check";

    public static void main(String[] args) {
        String user = "admin";
        String pass = "admin"; // get from user input
        String auth = user + ":"+ pass;

        String encoding = Base64.getEncoder().encodeToString((auth).getBytes(StandardCharsets.UTF_8)); // Encoding the username and password

        try { // sets time for the network
            long ut1 = Instant.now().getEpochSecond();
            String time = "http://192.168.1.103/time?TIME="+ut1;
            URL url = new URL (time); // URL connection
            HttpURLConnection connection = (HttpURLConnection) url.openConnection(); // Connecting to the specified URL
            connection.setRequestMethod("GET");
            connection.setDoOutput(true);
            connection.setRequestProperty("Authorization", "Basic " + encoding);
            InputStream content = (InputStream)connection.getContent(); // Getting the content - in this case text/plain
            BufferedReader in = new BufferedReader (new InputStreamReader (content));
            String line;
            while ((line = in.readLine()) != null) {
                System.out.println(line); // will need parsing
            }
        } catch(Exception e) {
            e.printStackTrace();
            return;
        }
        // Nodes list example
        try {
            URL url = new URL (nodes_list); // URL connection
            HttpURLConnection connection = (HttpURLConnection) url.openConnection(); // Connecting to the specified URL
            connection.setRequestMethod("GET");
            connection.setDoOutput(true);
            connection.setRequestProperty("Authorization", "Basic " + encoding);
            InputStream content = (InputStream)connection.getContent(); // Getting the content - in this case text/plain
            BufferedReader in = new BufferedReader (new InputStreamReader (content));
            String line;
            while ((line = in.readLine()) != null) {
                System.out.println(line);
            }
        } catch(Exception e) {
            e.printStackTrace();
        }
        String TICKET = "";
        // Request example
        try {
            String NODE = "lobitos";
            String BROADCAST = "READINGS";;
            String request = "http://192.168.1.103/request?NODE="+NODE+"&BROADCAST="+BROADCAST; // READINGS, SETTINGS OR ERRORS - ONLY READINGS WORKS AS INTENDED CURRENTLY
            URL url = new URL (request); // URL connection
            HttpURLConnection connection = (HttpURLConnection) url.openConnection(); // Connecting to the specified URL
            connection.setRequestMethod("GET");
            connection.setDoOutput(true);
            connection.setRequestProperty("Authorization", "Basic " + encoding);
            InputStream content = (InputStream)connection.getContent(); // Getting the content - in this case text/plain
            BufferedReader in = new BufferedReader (new InputStreamReader (content));
            String line;
            System.out.println(request);
            while ((line = in.readLine()) != null) {
                TICKET = line; // PRINTS TICK
            }
        } catch(Exception e) {
            e.printStackTrace();
        }
        System.out.println(TICKET);
        String get_request = "http://192.168.1.103/get-request?TICKET="+TICKET;
        System.out.println(get_request);


        try { // get the request we just made
            URL url = new URL (get_request); // URL connection
            HttpURLConnection connection = (HttpURLConnection) url.openConnection(); // Connecting to the specified URL
            connection.setRequestMethod("GET");
            connection.setDoOutput(true);
            connection.setRequestProperty("Authorization", "Basic " + encoding);
            InputStream content = (InputStream)connection.getContent(); // Getting the content - in this case text/plain
            BufferedReader in = new BufferedReader (new InputStreamReader (content));
            String line;
            while ((line = in.readLine()) != null) {
                System.out.println(line); // will need parsing
            }
        } catch(Exception e) {
            e.printStackTrace();
        }



    }

}
