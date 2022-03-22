import java.sql.Connection;
import java.sql.ResultSet;
import java.sql.SQLException;
import java.sql.Statement;
import java.util.ArrayList;
import java.util.List;

public class DataAccess
{
    private Connection sql_conn;
    public DataAccess(Connection sql_conn)
    {
        this.sql_conn = sql_conn;
    }
    public List<Room> getRoomInfo(String type, String identifier)
    {
        List<Room> roomList = new ArrayList<Room>();
        try {
            Statement statement = sql_conn.createStatement();
            String findRoom = "select * from room where "+type+"==\'"+identifier+"\';";
            ResultSet resultSet = statement.executeQuery(findRoom);
            while (resultSet.next())
            {

            }
        } catch (SQLException throwables) {
            throwables.printStackTrace();
        }
        return roomList;
    }
    public boolean addRoom(String roomId)
    {
        try {
            Statement statement = sql_conn.createStatement();
            String findRoom = "insert into room values;";
            statement.executeUpdate(findRoom);
            return true;
        } catch (SQLException throwables) {
            throwables.printStackTrace();
        }
        return false;
    }
}
