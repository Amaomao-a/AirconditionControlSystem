import java.io.BufferedReader;
import java.io.IOException;
import java.io.InputStreamReader;
import java.net.InetSocketAddress;
import java.net.UnknownHostException;
import java.nio.ByteBuffer;
import java.sql.*;
import java.time.Instant;
import java.util.Collections;
import java.util.HashMap;

import org.java_websocket.WebSocket;
import org.java_websocket.drafts.Draft;
import org.java_websocket.drafts.Draft_6455;
import org.java_websocket.handshake.ClientHandshake;
import org.java_websocket.server.WebSocketServer;

public class Server extends WebSocketServer
{
    HashMap<String,WebSocket> address_map;

    private Connection sql_conn;
    private Statement stmt;
    private Core core;

    public void setSql_conn(Connection sql_conn) {
        this.sql_conn = sql_conn;
    }
    public void setCore(Core core) {
        this.core = core;
    }

    public Server(int port) throws UnknownHostException
    {
        super(new InetSocketAddress(port));
    }

    public Server(InetSocketAddress address)
    {
        super(address);
    }

    public Server(int port, Draft_6455 draft)
    {
        super(new InetSocketAddress(port), Collections.<Draft>singletonList(draft));
    }

    @Override
    public void onOpen(WebSocket conn, ClientHandshake handshake)
    {
        //conn.send("Welcome to the server!"); //This method sends a message to the new client
        //broadcast("new connection: " + handshake.getResourceDescriptor()); //This method sends a message to all clients connected
        try
        {
            Statement statement = sql_conn.createStatement();
            //System.out.println("1  "+conn.getRemoteSocketAddress());
            String conn_addr =""+conn.getRemoteSocketAddress();//带端口号
            //String conn_addr = conn.getRemoteSocketAddress().getAddress().getHostAddress();//不带端口号
            String check_room = "select * from address where address = \'"+conn_addr+"\'";
            ResultSet res = statement.executeQuery(check_room);

            if (!res.next())
            {
                String add_room = "insert into address (address) " + "values(\'"+conn_addr+"\')";
                statement.executeUpdate(add_room);
                System.out.println("新链接登录，已录入数据库"+conn_addr);
                address_map.put(conn_addr,conn);
            }
            else
            {
                System.out.println("已有链接登录：" + conn_addr);
                String add_room = "update address set kind = 0 where address =\'"+conn_addr+"\'";
                statement.executeUpdate(add_room);
                address_map.put(conn_addr,conn);
            }
        } catch (SQLException throwables)
        {
            throwables.printStackTrace();
            System.out.println("database add room error");
        }
    }

    @Override
    public void onClose(WebSocket conn, int code, String reason, boolean remote)
    {
        //broadcast(conn + " has left the room!");
        System.out.println(conn.getRemoteSocketAddress().getAddress().getHostAddress() + " disconnected");
        String ip = '\''+""+conn.getRemoteSocketAddress()+'\'';//带端口号
        //String ip ='\''+conn.getRemoteSocketAddress().getAddress().getHostAddress()+'\'';
        try
        {
            Statement state = sql_conn.createStatement();
            String kind_check = "select * from address natural join user where address="+ip+";";
            ResultSet kind_check_res = state.executeQuery(kind_check);
            if (kind_check_res.next()&&kind_check_res.getInt("kind")==2)
            {
                //String state_check = "update user set state=0,address=null where address="+ip+";";
                //state.executeUpdate(state_check);
            }
        } catch (SQLException throwables)
        {
            throwables.printStackTrace();
        }
    }

    @Override
    public void onMessage(WebSocket conn, String message)
    {
        //broadcast(message);
        //String ip = conn.getRemoteSocketAddress().getAddress().getHostAddress();//不带端口号
        String ip = ""+conn.getRemoteSocketAddress();//带端口号
        System.out.println(conn + ": " + message);
        //conn.send(core.test(message));
        HashMap<String,String> ret;
        try
        {
            core.process(message,ip);
        } catch (SQLException throwables)
        {
            throwables.printStackTrace();
        }
    }

    @Override
    public void onMessage(WebSocket conn, ByteBuffer message)
    {
        //broadcast(message.array());
        System.out.println(conn + ": " + message);
    }

    @Override
    public void onError(WebSocket conn, Exception ex)
    {
        ex.printStackTrace();
        if (conn != null) {
            // some errors like port binding failed may not be assignable to a specific websocket
        }
    }

    @Override
    public void onStart()
    {
        System.out.println("Server started!");
//        try
//        {
//            InetAddress addr = InetAddress.getLocalHost();
//            System.out.println("Local HostAddress: "+addr.getAddress());
//        } catch (UnknownHostException e)
//        {
//            System.out.println("获取IP地址错误");
//            e.printStackTrace();
//        }
        setConnectionLostTimeout(0);
        setConnectionLostTimeout(100);
    }
    public boolean sendRoomMes(String ip,String message)
    {
        if (address_map.containsKey(ip))
        {
            address_map.get(ip).send(message);
            return true;
        }
        return false;
    }
}