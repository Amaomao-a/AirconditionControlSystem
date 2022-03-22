import com.alibaba.fastjson.JSONObject;
import org.java_websocket.WebSocket;

import java.io.BufferedReader;
import java.io.IOException;
import java.io.InputStreamReader;
import java.sql.Connection;
import java.sql.DriverManager;
import java.sql.SQLException;
import java.time.Instant;
import java.util.HashMap;

public class Main
{
    static final String JDBC_DRIVER = "com.mysql.cj.jdbc.Driver";
    //static final String DB_URL = "jdbc:mysql://10.128.206.75:3306/hotelinfo?useSSL=false&allowPublicKeyRetrieval=true&serverTimezone=GMT";
    //static final String DB_URL = "jdbc:mysql://localhost:3306/hotelinfo";
    static final String DB_URL = "jdbc:mysql://localhost:3306/hotelinfo";
    // 数据库的用户名与密码，需要根据自己的设置
    static final String USER = "root";
    //    static final String PASS = "newpassword";
    static final String PASS = "123456";
    static final int port = 7777;
    static final int SIZE = 3;
    static int MODE = 0;
    static double tempLow = 17.0;
    static double tempHigh = 26.0;
    static double rateFee = 1.0/60;
    static int defaultWind = 2;
    static Instant start;
    public static void main(String[] args) throws InterruptedException, IOException, ClassNotFoundException, SQLException
    {
        start = Instant.now();

        JSONObject localData = LocalData.getData();
        System.out.println("config:"+localData.toString());
        MODE = localData.getInteger("mode");
        tempLow = localData.getDouble("tempLow");
        tempHigh = localData.getDouble("tempHigh");
        rateFee = localData.getDouble("rateFee");
        defaultWind = localData.getInteger("defaultWind");
        //websocket
        // 843 flash policy port
        Server server = new Server(port);
        server.address_map = new HashMap<String, WebSocket>();

        //sql
        Class.forName(JDBC_DRIVER);
        System.out.println("connecting to database");
        Connection sql_conn = DriverManager.getConnection(DB_URL+"?useSSL=false&allowPublicKeyRetrieval=true&serverTimezone=UTC",USER,PASS);
        System.out.println("database connect successful");
        System.out.println("Local HostAddress: "+TestAddr.getAllLocalHostIP());

        //core
        Core core = new Core(sql_conn);

        //Schedule
        Schedule schedule = new Schedule(SIZE,server,core);

        //input
        core.setServer(server);
        core.setStart(start);
        core.setSchedule(schedule);
        server.setCore(core);
        server.setSql_conn(sql_conn);
        //server start
        server.start();
        System.out.println("Server started on port: " + server.getPort());
        schedule.run();

    }
}
