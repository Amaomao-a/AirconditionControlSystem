import org.java_websocket.WebSocket;
import com.alibaba.fastjson.JSON;
import com.alibaba.fastjson.JSONObject;

import java.nio.charset.Charset;
import java.nio.charset.StandardCharsets;
import java.sql.*;
import java.time.Duration;
import java.time.Instant;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.Map;
import java.util.Random;

public class Core
{
    private Connection sql_conn;
    private Server server;
    private Schedule schedule;
    //static final String seed = "DASNIOCBO82OGVA9";
    private Instant start;

    public void setSchedule(Schedule schedule) {
        this.schedule = schedule;
    }
    public void setStart(Instant start) {
        this.start = start;
    }
    public Core(Connection sql_conn)
    {
        this.sql_conn = sql_conn;
    }
    public Connection getSql_conn()
    {
        return sql_conn;
    }
    public void setSql_conn(Connection sql_conn)
    {
        this.sql_conn = sql_conn;
    }
    public void setServer(Server server){this.server=server;}
    public void process(String message,String ip) throws SQLException
    {
        JSONObject mes = JSON.parseObject(message);
        //System.out.println("11111");
        JSONObject ans = new JSONObject();
        String sql_ip = "\'"+ip+"\'";
        if (!mes.containsKey("refId"))
        {
            replyWithRefId(mes,errorMes("消息不符合格式"),ip);
            return;
        }
        if (mes.containsKey("handler"))
        {
            //System.out.println((String) mes.get("handler"));
            String handler = (String)mes.get("handler");
            Statement state = sql_conn.createStatement();
            String query = "select * from address where address = "+sql_ip+";";
            //System.out.println(sql_ip);
            ResultSet type = state.executeQuery(query);
            type.next();
            int kind = type.getInt("kind");
            if (handler.startsWith("/manager")||handler.startsWith("/admin"))
            {
                if (kind == 0)
                {
                    state.executeUpdate("update address set kind = 2 where address ="+sql_ip+";");
                    state.executeUpdate("insert into manager (address) values ("+sql_ip+");");
                }
                if (kind == 1)
                {
                    replyWithRefId(mes,errorMes("客户端不能使用管理员端指令"),ip);
                    return;
                }
                else
                {
                    switch (handler)
                    {
                        case "/manager/login":
                            login(mes,ip);
                            break;
                        case "/manager/logout":
                            logout(mes,ip);
                            break;
                        case "/manager/openRoom":
                            openRoom(mes,ip);
                            break;
                        case "/manager/getRoomList":
                            getRoomList(mes,ip);
                            break;
                        case "/manager/seeRoomInfo":
                            seeRoomInfo(mes,ip);
                            break;
                        case "/manager/controlRoom":
                            controlRoom(mes,ip);
                            break;
                        case "/manager/closeRoom":
                            closeRoom(mes,ip);
                        case "/admin/simpleCost":
                            break;
                        case "/admin/detailCost":
                            break;
                        case "/admin/sendMsg":
                            break;
                        case "/manager/detailCost":
                            detailCost(mes,ip);
                            break;
                        case "/manager/detailCostFile":
                            break;
                    }
                }
            }
            else if (handler.startsWith("/client"))
            {
                if (kind == 0)
                    state.executeUpdate("update address set kind = 1 where address ="+sql_ip+";");
                if (kind == 2)
                {
                    replyWithRefId(mes,errorMes("客户端不能使用管理员端指令"),ip);
                    return;
                }
                else
                {
                    switch (handler)
                    {
                        case "/client/init":
                            initRoom(mes,ip);
                            break;
                        case "/client/open":
                            openAC(mes,ip);
                            break;
                        case "/client/controlRoom":
                            setAC(mes,ip);
                            break;
                        case "/client/requestState":
                            updateStatus(mes,ip);
                            break;
                        case "/client/error":
                            break;
                        case "/client/contact":
                            contact(mes,ip);
                            break;
                        case "/client/confirm":
                            break;
                        case "/client/tempUp":
                            tempUp(mes,ip);
                            break;
                        case "/client/tempRelease":
                            tempRelease(mes,ip);
                            break;
                    }
                }

            }
            else
            {
                replyWithRefId(mes,errorMes("未知类型handler"),ip);
                return;
            }
        }
        else
        {
            replyWithRefId(mes,errorMes("消息不符合格式"),ip);
            return;
        }
    }
    private void detailCost(JSONObject message,String ip) throws SQLException
    {
        JSONObject data = message.getJSONObject("data");
        String roomId = data.getString("roomId");
        Statement statement = sql_conn.createStatement();
        String getLog = "select * from money_log where room_num =\'"+roomId+"\' order by room_num";
        ResultSet resultSet = statement.executeQuery(getLog);
        JSONObject ansData = new JSONObject();
        ArrayList<JSONObject> list = new ArrayList<JSONObject>();
        while (resultSet.next())
        {
            JSONObject tuple = new JSONObject();
            tuple.put("roomId",resultSet.getString("room_num"));
            tuple.put("beginTime",resultSet.getInt("begin_time"));
            tuple.put("endTime",resultSet.getInt("end_time"));
            tuple.put("money",resultSet.getDouble("money"));
            tuple.put("sum",resultSet.getDouble("sum"));
            tuple.put("wind",resultSet.getInt("wind"));
            list.add(tuple);
        }
        ansData.put("data",list);
        ansData.put("handler","/server/detailCost");
        replyWithRefId(message,ansData,ip);
    }
    private void closeRoom(JSONObject message,String ip) throws SQLException
    {
        JSONObject data = message.getJSONObject("data");
        changeWind(data.getString("roomId"),0);
        //Statement statement = sql_conn.createStatement();
        //statement.executeUpdate(resetRoom);
        schedule.releaseRoom(data.getString("roomId"));
        sendRoomStop(data.getString("roomId"));
        replyWithRefId(message,ack(),ip);
    }
    private void tempUp(JSONObject message,String ip) throws SQLException
    {
        JSONObject data = message.getJSONObject("data");
        String room_num = data.getString("roomId");
        schedule.releaseRoom(room_num);
        changeWind(room_num,0);
        sendRoomStop(room_num);
        replyWithoutRefId(changeReply(-1,-1,0),ip);
        replyWithRefId(message,ack(),ip);
    }
    private void tempRelease(JSONObject message,String ip) throws SQLException
    {
        JSONObject data = message.getJSONObject("data");
        String room_num = data.getString("roomId");
        int requestWind = data.getInteger("wind");
        int currentWind = schedule.roomRequest(room_num,requestWind);
        data.put("wind",currentWind);
        changeWind(room_num,currentWind);
        sendRoomOpen(room_num,currentWind);
        JSONObject reply = new JSONObject();
        reply.put("data",data);
        reply.put("handler","/server/tempRelease");
        replyWithRefId(message,reply,ip);
        JSONObject changeReply = changeReply(-1,-1,currentWind);
        replyWithoutRefId(changeReply,ip);
    }
    private boolean judgeRole(JSONObject message,int role) throws SQLException
    {
        String token = '\''+message.getString("token")+'\'';
        Statement state = sql_conn.createStatement();
        String role_check = "select * from manager where token="+token+";";
        ResultSet role_check_res = state.executeQuery(role_check);
        if (role_check_res.next() && role_check_res.getInt("role")==role)
        {
            return true;
        }
        else
        {
            return false;
        }
    }
    private void replyWithRefId(JSONObject recMessage,JSONObject returnMessage,String ip)
    {
        returnMessage.put("refId",recMessage.get("refId"));
        server.sendRoomMes(ip,returnMessage.toString());
    }
    private void replyWithoutRefId(JSONObject returnMessage,String ip)
    {
        server.sendRoomMes(ip,returnMessage.toString());
    }
    private void updateStatus(JSONObject message,String ip) throws SQLException
    {
        Statement state = sql_conn.createStatement();
        JSONObject data = (JSONObject) message.get("data");
        String room_num ='\''+data.getString("roomId")+'\'';
        String tmp_update = "update air_con set enviro_temp = "+data.getDouble("currentTmp")+" where room_num ="+room_num+";";
        state.executeUpdate(tmp_update);
        String money_check = "select * from air_con where room_num="+room_num+";";
        ResultSet money_res = state.executeQuery(money_check);
        JSONObject ans = new JSONObject();
        JSONObject ans_data = new JSONObject();
        if (money_res.next())
        {
            double money = money_res.getDouble("money_sum");
            int now_time = (int) Duration.between(start,Instant.now()).toSeconds();
            int b_wind = money_res.getInt("wind_level");
            double new_money = 0;
            if (b_wind != 0)
            {
                String check = "select * from money_log where room_num="+room_num+" and end_time is null;";
                ResultSet resultSet = state.executeQuery(check);
                resultSet.next();
                int begin_time = resultSet.getInt("begin_time");
                int wind_level = resultSet.getInt("wind");
                new_money = getMoney(begin_time,now_time,wind_level);
            }
            ans_data.put("totalFee",money+new_money);
            ans_data.put("currentFee",new_money);
        }
        ans.put("data",ans_data);
        ans.put("handler","/server/update");
        System.out.println(ans.toString());
        replyWithRefId(message,ans,ip);
    }

    private void changeWind(String room_num,int wind) throws SQLException
    {
        Statement state = sql_conn.createStatement();
        room_num = '\''+room_num + '\'';
        String room_num_check = "select * from air_con natural join room where room_num="+room_num+";";
        ResultSet room_num_res = state.executeQuery(room_num_check);
        room_num_res.next();
        if (room_num_res.getInt("power")==1)
        {
            int b_time = room_num_res.getInt("begin_time");
            int e_time = (int) Duration.between(start,Instant.now()).toSeconds();
            int b_wind = room_num_res.getInt("wind_level");
            double new_money = getMoney(b_time,e_time,b_wind);
            double sum_money = new_money+room_num_res.getDouble("money_sum");
            String set = "update air_con set money_sum="+sum_money+",wind_level="+wind+" ,begin_time="+e_time+" " +
                         "where room_num="+room_num+";";
            state.executeUpdate(set);
            if (b_wind != 0)
            {
                String update = "update money_log set end_time="+e_time+",money="+new_money+",sum="+sum_money+
                        " where room_num="+room_num+" and begin_time="+b_time+" and end_time is null;";
                state.executeUpdate(update);
            }
            if (wind != 0)
            {
                String insert = "insert into money_log(room_num, begin_time, wind, money, sum) " +
                        "VALUES (" + room_num + "," + e_time + "," + wind + ",0,0);";
                state.executeUpdate(insert);
            }
        }
        else
        {
            String set = "update air_con set wind_level="+wind+" where room_num="+room_num+";";
            state.executeUpdate(set);
        }
    }
    private void controlRoom(JSONObject message,String ip) throws SQLException
    {
        setAC(message,ip);
    }

    private void seeRoomInfo(JSONObject message,String ip) throws SQLException
    {
        Statement state = sql_conn.createStatement();
        JSONObject data = (JSONObject) message.get("data");
        JSONObject ans = new JSONObject();
        ArrayList<JSONObject> roomInfoList = new ArrayList<JSONObject>();
        if (data.get("roomId")=="All")
        {
            String room_info_check = "select * from room natural join air_con where idle=0";
            ResultSet room_info_res = state.executeQuery(room_info_check);
            while (room_info_res.next())
            {
                JSONObject room = new JSONObject();
                room.put("roomId",room_info_res.getString("room_num"));
                room.put("power",room_info_res.getInt("power"));
                room.put("wind",room_info_res.getInt("wind_level"));
                room.put("setTmp",room_info_res.getInt("temperature"));
                room.put("nowTmp",room_info_res.getInt("enviro_temp"));
                roomInfoList.add(room);
            }
        }
        else
        {
            String room_num = '\''+(String) data.get("roomId")+'\'';
            String room_info_check = "select * from room natural join air_con where idle=0 and room_num="+room_num+";";
            ResultSet room_info_res = state.executeQuery(room_info_check);
            if (room_info_res.next())
            {
                JSONObject room = new JSONObject();
                room.put("roomId",room_info_res.getString("room_num"));
                room.put("power",room_info_res.getInt("power"));
                room.put("wind",room_info_res.getInt("wind_level"));
                room.put("setTmp",room_info_res.getInt("temperature"));
                room.put("nowTmp",room_info_res.getInt("enviro_temp"));
                roomInfoList.add(room);
            }
        }
        JSONObject retData = new JSONObject();
        retData.put("roomInfoList",roomInfoList);
        ans.put("data",retData);
        ans.put("handler","/server/seeRoomInfo");
        replyWithRefId(message,ans,ip);
    }

    private String getRoomNum(JSONObject message) throws SQLException
    {
        Statement state = sql_conn.createStatement();
        JSONObject data = (JSONObject) message.get("data");

        String token = "\'"+(String) data.get("token")+"\'";
        String room_num_check1 = "select room_num from air_con natural join room where token="+token+";";
        ResultSet room_num_res1 = state.executeQuery(room_num_check1);
        if (room_num_res1.next())
            return '\''+room_num_res1.getString("room_num")+'\'';
        else
            return null;
    }
    private void contact(JSONObject message,String ip) throws SQLException
    {
        System.out.println("message: "+message);
        System.out.println("from room: "+getRoomNum(message));
        replyWithRefId(message,ack(),ip);
    }
    private void getRoomList(JSONObject message,String ip) throws SQLException
    {
        Statement state = sql_conn.createStatement();
        JSONObject ans = new JSONObject();
        JSONObject data = new JSONObject();
        ans.put("handler","/server/retRoomList");
        ArrayList<JSONObject> roomList = new ArrayList<JSONObject>();
        String room_check = "select * from room";
        ResultSet res = state.executeQuery(room_check);
        while (res.next())
        {
            JSONObject room = new JSONObject();
            room.put("roomId",res.getString("room_num"));
            if ((int)res.getInt("idle")==0)
                room.put("idle",false);
            else
                room.put("idle",true);
            roomList.add(room);
        }
        data.put("roomList",roomList);
        ans.put("data",data);
        replyWithRefId(message,ans,ip);
    }
    private void setAC(JSONObject message,String ip) throws SQLException
    {
        Statement state = sql_conn.createStatement();
        JSONObject data = (JSONObject) message.get("data");
        String room_num = '\''+data.getString("roomId")+'\'';
        int power = (int)data.get("power");
        if (power!=-1)
        {
            if (power==1)
            {
                int temp;
                String de_tmp_check = "select * from room where room_num="+room_num+";";
                ResultSet de_tmp_res = state.executeQuery(de_tmp_check);
                de_tmp_res.next();
                temp = de_tmp_res.getInt("default_temp");
                long time = Duration.between(start,Instant.now()).toSeconds();
                String set_power = "update air_con set power=1,temperature="+temp+",wind_level=0,begin_time="+time+" where room_num="+room_num+";";
                String insert_log = "insert into money_log values ("+room_num+","+time+",NULL,0,0,0)";
                state.executeUpdate(set_power);
                state.executeUpdate(insert_log);
                JSONObject reply = changeReply(1,-1,-1);
                replyWithoutRefId(reply,ip);
            }
            else if (power==0)
            {
                changeWind(data.getString("roomId"),0);
                String set_power = "update air_con set power=0 where room_num="+room_num+";";
                state.executeUpdate(set_power);
                JSONObject reply = changeReply(0,-1,-1);
                replyWithoutRefId(reply,ip);
                schedule.releaseRoom(room_num);
            }
        }
        if ((int)data.get("tmp")!=-1)
        {
            int tmp = (int)data.get("tmp");
            String set_tmp = "update air_con set temperature="+tmp+" where room_num ="+room_num+";";
            JSONObject reply = changeReply(-1,tmp,-1);
            replyWithoutRefId(reply,ip);
            state.executeUpdate(set_tmp);
        }
        if ((int)data.get("wind")!=-1)
        {
            int wind = (int)data.get("wind");
            int currentWind = schedule.roomRequest(data.getString("roomId"),wind);
            changeWind(data.getString("roomId"),currentWind);
            sendRoomOpen(data.getString("roomId"),currentWind);
        }
        replyWithRefId(message,ack(),ip);
    }
    private JSONObject changeReply(int power,int tmp,int wind)
    {
        JSONObject data = new JSONObject();
        data.put("power",power);
        data.put("mode",Main.MODE);
        data.put("tempLow",Main.tempLow);
        data.put("tempHigh",Main.tempHigh);
        data.put("tmp",tmp);
        data.put("wind",wind);
        data.put("rateFee",Main.rateFee);
        JSONObject reply = new JSONObject();
        reply.put("data",data);
        reply.put("handler","/server/change");
        reply.put("refID",generateRefId());
        return reply;
    }
    private double getMoney(int begin,int end,int wind)
    {
        if (wind==3) return (end-begin) * Main.rateFee;
        else if (wind==2) return (end-begin) * Main.rateFee * 1/2;
        else if (wind==1) return (end-begin) * Main.rateFee * 1/3;
        else return 0;
    }
    private void openRoom(JSONObject message, String ip) throws SQLException
    {
        Statement state = sql_conn.createStatement();
        JSONObject data = (JSONObject) message.get("data");
        //还没添加defaultTemp功能
        String room_num = '\''+data.getString("roomId")+'\'';
        String token = generateToken();
        String sql_token = "\'"+token+"\'";
        String set_room = "update room set idle = 0,token="+sql_token+" where room_num="+room_num+";";
        state.executeUpdate(set_room);
        String room_ip_query = "select address from room where room_num="+room_num+";";
        ResultSet ip_res = state.executeQuery(room_ip_query);
        ip_res.next();
        String room_ip = ip_res.getString("address");

        JSONObject room_mes = new JSONObject();
        room_mes.put("refId",generateRefId().toString());
        room_mes.put("handler","/server/openRoom");
        room_mes.put("token",token);
        JSONObject send_data = new JSONObject();
        send_data.put("token",token);
        send_data.put("defaultTmp",data.get("defaultTmp"));
        send_data.put("defaultWind",Main.defaultWind);
        send_data.put("tempLow",Main.tempLow);
        send_data.put("tempHigh",Main.tempHigh);
        send_data.put("mode",Main.MODE);

        room_mes.put("data",send_data);
        replyWithoutRefId(room_mes,room_ip);
        replyWithRefId(message,ack(),ip);
    }

    private void logout(JSONObject message,String ip) throws SQLException
    {
        Statement state = sql_conn.createStatement();
        JSONObject data = (JSONObject) message.get("data");
        String logout = "update manager set role = 0 where address =\'"+ip+"\';";
        state.executeUpdate(logout);
        String user_logout = "update user set state = 0 where user_name =\'"+data.get("adminId")+"\';";
        state.executeUpdate(user_logout);
        replyWithRefId(message,ack(),ip);
    }
    private void login(JSONObject message,String ip) throws SQLException
    {
        Statement state = sql_conn.createStatement();
        JSONObject data = (JSONObject) message.get("data");
        String login_check = "select * from address where address=\'"+ip+"\';";
        ResultSet mana_state = state.executeQuery(login_check);
        mana_state.next();
        JSONObject ret = new JSONObject();
        JSONObject ret_data = new JSONObject();
        ret.put("handler","/server/retRole");
        String token = generateToken();
        if (mana_state.getInt("kind")!=2)//若发包的不是管理员端
        {
            replyWithRefId(message,errorMes("客户端不允许登录"),ip);
            return;
        }
        else
        {
            String name = "\'"+data.get("adminId")+"\'";
            String pass = "\'"+data.get("password")+"\'";
            //String user_check = "select * from user where user_name="+name+" and user_password="+pass+" and state=0;";
            String user_check = "select * from user where user_name="+name+" and user_password="+pass+";";

            ResultSet res = state.executeQuery(user_check);
            //System.out.println(res.getString("user_name"));
            String sql_token = "\'"+token+"\'";
            if (res.next())
            {
                int level = res.getInt("user_level");
                String user_login = "update manager set role="+level+",token="+sql_token+" where address=\'"+ip+"\';";
//                String user_login2 = "update user set state=1"+",address="+ip+" where user_name="+name+";";
                String user_login2 = "update user set state=1"+" where user_name="+name+";";
                state.executeUpdate(user_login);
                state.executeUpdate(user_login2);
                ret_data.put("result",true);
                if (level == 1)
                    ret_data.put("role","manager");
                if (level == 2)
                    ret_data.put("role","admin");
            }
            else//账户或者密码不正确或者当前账户已经被登录
            {
                replyWithRefId(message,errorMes("登录失败"),ip);
                return;
            }
        }
        ret_data.put("token",token);
        ret.put("data",ret_data);
        replyWithRefId(message,ret,ip);
    }
    private void initRoom(JSONObject message,String ip) throws SQLException
    {
        JSONObject data = (JSONObject) message.get("data");
        long b_time = Duration.between(start,Instant.now()).toSeconds();
        String roomId = '\''+(String)data.get("roomId")+'\'';
        String room_check = "select * from room where room_num="+roomId+";";
        String init_room = "insert into room(address,room_num,default_temp) " + "values(\'"+ip+"\',"+roomId+","+data.get("initTmp")+");";
        String init_ac = "insert into air_con(room_num,begin_time,power) " + "values("+roomId+","+b_time+",0);";
        Statement state = sql_conn.createStatement();
        ResultSet room_check_res = state.executeQuery(room_check);
        if (room_check_res.next())
        {
            String update_room = "update room set address=\'"+ip+"\' where room_num="+roomId+";";
            state.executeUpdate(update_room);
        }
        else
        {
            state.executeUpdate(init_room);
            state.executeUpdate(init_ac);
        }
        replyWithRefId(message,ack(),ip);
    }

    private void openAC(JSONObject message,String ip) throws SQLException//useless
    {
        Statement state = sql_conn.createStatement();
        String room_num = getRoomNum(message);
        String AC_check = "select power from air_con where room_num ="+room_num+";";
        ResultSet ac_power = state.executeQuery(AC_check);
        ac_power.next();
        if (ac_power.getInt("power") == 0)
        {
            String enviro_temp = "select default_temp from room where room_num ="+room_num+";";
            ResultSet temp = state.executeQuery(enviro_temp);
            temp.next();
            int a_temp = temp.getInt("default_temp");
            String open_ac = "update air_con set power = 1,wind_level = 2,enviro_temp = "+a_temp+" where room_num =" +room_num+";";
            state.executeUpdate(open_ac);
            replyWithRefId(message,ack(),ip);
        }
        else
            replyWithRefId(message,errorMes("空调已经开启"),ip);
    }

    private JSONObject errorMes(String report)
    {
        JSONObject error = new JSONObject();
        error.put("handler","/server/error");
        JSONObject data = new JSONObject();
        data.put("msg",report);
        error.put("data",data);
        return error;
    }

    private JSONObject ack()
    {
        JSONObject ack = new JSONObject();
        ack.put("handler","/server/confirm");
        return ack;
    }

    private String generateToken()
    {
        String str = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
        Random random = new Random();
        StringBuffer sb = new StringBuffer();
        for (int i = 0; i < 16; i++)
        {
            int num = random.nextInt(62);
            sb.append(str.charAt(num));
        }
        return sb.toString();
    }

    private Integer generateRefId()
    {
        Random random = new Random();
        return random.nextInt((int) Math.pow(10,9));
    }

    public void sendRoomOpen(String roomId,int windLevel) throws SQLException
    {
        String ip = findRoomIp(roomId);
        changeWind(roomId,windLevel);
        JSONObject openMes = new JSONObject();
        openMes.put("handler","/server/change");
        JSONObject data = new JSONObject();
        data.put("power",-1);
        data.put("tmp",-1);
        data.put("wind",windLevel);
        data.put("tempLow",-1);
        data.put("tempHigh",-1);
        data.put("rateFee",-1);
        openMes.put("data",data);
        openMes.put("refId",Integer.toString(generateRefId()));
        System.out.println(openMes.toString());
        replyWithoutRefId(openMes,ip);
    }
    public void sendRoomStop(String roomId) throws SQLException
    {
        changeWind(roomId,0);
        JSONObject stopMes = new JSONObject();
        stopMes.put("handler","/server/change");
        JSONObject data = new JSONObject();
        data.put("power",-1);
        data.put("tmp",-1);
        data.put("wind",0);
        data.put("tempLow",-1);
        data.put("tempHigh",-1);
        data.put("rateFee",-1);
        stopMes.put("refId",Integer.toString(generateRefId()));
    }
    public String findRoomIp(String roomId) throws SQLException
    {
        Statement statement = sql_conn.createStatement();
        String findRoom = "select * from room where room_num =\'"+roomId+"\';";
        ResultSet resultSet = statement.executeQuery(findRoom);
        if (resultSet.next())
        {
            return resultSet.getString("address");
        }
        else
        {
            return null;
        }
    }
}
