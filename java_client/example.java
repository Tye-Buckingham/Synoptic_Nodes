
import java.io.BufferedReader;
import java.io.InputStream;
import java.io.InputStreamReader;
import java.net.HttpURLConnection;
import java.net.URL;
import java.nio.charset.StandardCharsets;
import java.sql.Time;
import java.sql.Timestamp;
import java.time.Instant;
import java.time.LocalDateTime;
import java.util.*;
import java.util.stream.Collectors;
import java.util.stream.Stream;

import org.json.JSONArray;
import org.json.JSONException;
import org.json.JSONObject;
import org.json.JSONString;
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

    public static int Turbidity = 0;
    public static int TDS = 1;
    public static int pressure = 2;
    public static int flow_rate = 3;
    public static int pH = 4;
    public static int Temp = 5;

    public static HashMap<Timestamp, List<Integer>> readings = new HashMap<Timestamp, List<Integer>>();

    /**
     * Method to parse a request for readings from a node
     *
     * @param readings_string the request returned to the client
     *                        formtted: {"ticket":5,"1622632039": [1,557,22,2,29], ... "1622730328": [1,438,22,13,25]}
     */
    private static void addReadings(String readings_string) {

        JSONObject jsonObject = new JSONObject(readings_string.trim());
        Iterator<String> keys = jsonObject.keys();

        while (keys.hasNext()) {
            String key = keys.next();
            if (!key.equals("ticket")) {
                System.out.println(jsonObject.get(key));
                List<Integer> temp_readings = Stream.of(jsonObject.get(key).toString().replaceAll("[\\[\\]]", "").split(",")).map(Integer::parseInt).collect(Collectors.toList());
                readings.put(new Timestamp(Long.parseLong(key)), temp_readings);
            }
        }
    }

    /**
     * Method to sync the time with the client application
     *
     * @param time the current time of the client as a unix timestamp
     * @param encoding the encoding needed for verification
     */
    private static void syncTime(String time, String encoding) {
        try { // sets time for the network
            long ut1 = Instant.now().getEpochSecond();
            time = "http://192.168.1.103/time?TIME=" + ut1;
            URL url = new URL(time); // URL connection
            HttpURLConnection connection = (HttpURLConnection) url.openConnection(); // Connecting to the specified URL
            connection.setRequestMethod("GET");
            connection.setDoOutput(true);
            connection.setRequestProperty("Authorization", "Basic " + encoding);
            InputStream content = (InputStream) connection.getContent(); // Getting the content - in this case text/plain
            BufferedReader in = new BufferedReader(new InputStreamReader(content));
            String line;
            while ((line = in.readLine()) != null) {
                System.out.println(line); // will need parsing
            }
        } catch (Exception e) {
            e.printStackTrace();
            return;
        }
    }

    /**
     * Method to return the list of currently connect nodes on the network
     * @param encoding the encoding needed for verification
     */
    private static void listNodes(String encoding) {
        try {
            URL url = new URL(nodes_list); // URL connection
            HttpURLConnection connection = (HttpURLConnection) url.openConnection(); // Connecting to the specified URL
            connection.setRequestMethod("GET");
            connection.setDoOutput(true);
            connection.setRequestProperty("Authorization", "Basic " + encoding);
            InputStream content = (InputStream) connection.getContent(); // Getting the content - in this case text/plain
            BufferedReader in = new BufferedReader(new InputStreamReader(content));
            String line;
            while ((line = in.readLine()) != null) {
                System.out.println(line);
            }
        } catch (Exception e) {
            e.printStackTrace();
        }
    }

    /**
     * Method to make a request on the network allowing for READINGS, READ_ARCHIVE and SETTINGS
     * @param NODE the node the message is to be sent to as its string name
     * @param BROADCAST the request type (listed above)
     * @param encoding the encoding needed for verification
     * @return String the ticket number received from the server
     */
    private static String makeRequest(String NODE, String BROADCAST, String encoding) {
        String TICKET = "";
        try {
            NODE = "lobitos";
            BROADCAST = "READINGS";
            ;
            String request = "http://192.168.1.103/request?NODE=" + NODE + "&BROADCAST=" + BROADCAST; // READINGS, SETTINGS OR ERRORS - ONLY READINGS WORKS AS INTENDED CURRENTLY
            URL url = new URL(request); // URL connection
            HttpURLConnection connection = (HttpURLConnection) url.openConnection(); // Connecting to the specified URL
            connection.setRequestMethod("GET");
            connection.setDoOutput(true);
            connection.setRequestProperty("Authorization", "Basic " + encoding);
            InputStream content = (InputStream) connection.getContent(); // Getting the content - in this case text/plain
            BufferedReader in = new BufferedReader(new InputStreamReader(content));
            String line;
            System.out.println(request);
            while ((line = in.readLine()) != null) {
                TICKET = line; // PRINTS TICK
            }
        } catch (Exception e) {
            e.printStackTrace();
        }
        return TICKET;
    }

    /**
     * Method to clear all tickets and their data currently stored on the server
     * @param encoding the encoding needed for verification
     */
    private static void clearTickets(String encoding) {
        try {

            String request = "http://192.168.1.103/clear-tickets"; // READINGS, SETTINGS OR ERRORS - ONLY READINGS WORKS AS INTENDED CURRENTLY
            URL url = new URL(request); // URL connection
            HttpURLConnection connection = (HttpURLConnection) url.openConnection(); // Connecting to the specified URL
            connection.setRequestMethod("GET");
            connection.setDoOutput(true);
            connection.setRequestProperty("Authorization", "Basic " + encoding);
            InputStream content = (InputStream) connection.getContent(); // Getting the content - in this case text/plain
            BufferedReader in = new BufferedReader(new InputStreamReader(content));
            String line;
            System.out.println(request);
            while ((line = in.readLine()) != null) {
            }
        } catch (Exception e) {
            e.printStackTrace();
        }
    }

    /**
     * Method to get the request from the server after making one and storing the provided ticket number
     * @param TICKET the ticket number received from the server when making a request
     * @param encoding the encoding needed for verification
     * @return String the reply from the server as a JSON formatted plain text string
     */
    private static String getRequest(String TICKET, String encoding) {
        String get_request = "http://192.168.1.103/get-request?TICKET=" + TICKET;
        String line = "";
        try { // get the request we just made
            URL url = new URL(get_request); // URL connection
            HttpURLConnection connection = (HttpURLConnection) url.openConnection(); // Connecting to the specified URL
            connection.setRequestMethod("GET");
            connection.setDoOutput(true);
            connection.setRequestProperty("Authorization", "Basic " + encoding);
            InputStream content = (InputStream) connection.getContent(); // Getting the content - in this case text/plain
            BufferedReader in = new BufferedReader(new InputStreamReader(content));
            while ((line = in.readLine()) != null) {
                System.out.println(line); // will need parsing
            }
        } catch (
                Exception e) {
            e.printStackTrace();
        }
        return line;
    }


    public static void main(String[] args) {
        String user = "admin";
        String pass = "admin"; // get from user input
        String auth = user + ":"+ pass;

        String encoding = Base64.getEncoder().encodeToString((auth).getBytes(StandardCharsets.UTF_8)); // Encoding the username and password

        // Doesn't account for duplicates - may need to be handled on node's side
        String test = "{\"ticket\":5,\"1622632444\": [1,557,22,2,29],\"1622632044\": [1,1447,23,2,29]," +
                "\"1622632031\": [1,557,22,2,29],\"1622632267\": [1,1223,22,6,26],\"1622632245\": [1,1521,25,12,24]," +
                "\"1622632111\": [1,560,25,4,29],\"1622632131\": [0,1417,25,12,27],\"1622632467\": [1,1189,24,3,28]," +
                "\"1622632000\": [2,834,23,10,28],\"1622632009\": [2,740,24,3,25],\"1622632246\": [0,1201,25,6,23]," +
                "\"1622632099\": [1,318,22,1,23],\"1622632254\": [1,853,22,8,25],\"1622632686\": [2,379,25,3,28]," +
                "\"1622632112\": [0,1924,21,10,23],\"1622632589\": [2,505,24,6,26],\"1622632159\": [1,1869,25,5,23]," +
                "\"1622729368\": [0,1424,21,2,24],\"1622729428\": [0,1924,21,10,23],\"1622729488\": [2,1843,25,0,25]," +
                "\"1622729548\": [1,303,21,0,29],\"1622729608\": [1,606,22,10,20],\"1622729668\": [1,1872,21,9,20]," +
                "\"1622729728\": [1,1740,24,7,28],\"1622729788\": [1,1806,25,7,23],\"1622729848\": [0,574,24,7,26]," +
                "\"1622729908\": [1,1454,22,13,26],\"1622729968\": [0,1415,23,11,27],\"1622730028\": [0,1453,21,9,28]," +
                "\"1622730088\": [1,958,23,7,21],\"1622730148\": [1,1041,21,2,22],\"1622730208\": [1,629,22,4,28]," +
                "\"1622730268\": [2,1380,24,4,26],\"1622730328\": [1,438,22,13,25]}";

        addReadings(test);
        for(Map.Entry<Timestamp, List<Integer>> entry : readings.entrySet()) {
            System.out.println("Key: " + entry.getKey().toLocalDateTime().toString() + " Value: " + entry.getValue().toString());
        }
    }


}
