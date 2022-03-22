import java.sql.Connection;
import java.sql.SQLException;
import java.util.*;

import static java.lang.Thread.getAllStackTraces;
import static java.lang.Thread.sleep;

public class Schedule implements Runnable
{
    static int TIME = 2;
    private int size;
    private int usedSize;
    public boolean flag;
    public List<Room> state;
    private Server server;
    private Core core;
    public Schedule(int size,Server server,Core core)
    {
        this.size = size;
        this.usedSize = 0;
        this.state = new ArrayList<Room>();
        this.server = server;
        this.core = core;
        this.flag = false;
    }
    public Schedule(int size)
    {
        this.size = size;
        this.usedSize = 0;
        this.state = new ArrayList<Room>();
        this.flag = false;
    }
    @Override
    public void run()
    {
        flag = true;
        while (flag)
        {
            Collections.sort(state);
            for (Room room:state)
            {
//                if (room.serveTime == 0 && room.state == 1)
//                {
//                    //转到waitting状态
//                    room.state = 0;
//                    usedSize --;
//                    sendStop(room.roomId);
//                }
                if (room.waitingTime <= 0 && room.state == 0)
                {
                    if (state.get(0).requestWindLevel <= room.requestWindLevel)
                    {
                        room.state = 1;
                        room.waitingTime = 0;
                        room.serveTime = 0;
                        usedSize++;
                        sendOpen(room.roomId,room.requestWindLevel);
                    }
                }
            }
            popRoom();
            report();
            try {
                sleep(60000);
            } catch (InterruptedException e) {
                e.printStackTrace();
            }
            timeCost();
        }
    }
    public void sendStop(String roomId)
    {
        try {
            core.sendRoomStop(roomId);
        } catch (SQLException throwables) {
            throwables.printStackTrace();
        }
    }
    public void sendOpen(String roomId,int windLevel)
    {
        try {
            core.sendRoomOpen(roomId,windLevel);
        } catch (SQLException throwables) {
            throwables.printStackTrace();
        }
    }
    private void popRoom()
    {
        if (state.size() == 0) return;
        while (usedSize > size)
        {
            Collections.sort(state);
            if (state.get(0).state == 0)
                break;
            state.get(0).state = 0;
            state.get(0).waitingTime = Schedule.TIME;
            state.get(0).serveTime = 0;
            sendStop(state.get(0).roomId);
            usedSize --;
        }
    }
    public void nextRoom()
    {
//        while (usedSize < size)
//        {
//            Collections.sort(state);
//
//        }
    }
    public int roomRequest(String roomId,int windLevel)
    {
        Room room = new Room(roomId,windLevel);
        for (Room i:state)
        {
            if (i.roomId.equals(roomId))
            {
                releaseRoom(roomId);
                break;
            }
        }
        if (usedSize < size)
        {
            room.state = 1;
            room.serveTime = 0;
            room.waitingTime = 0;
            state.add(room);
            usedSize ++;
            return room.requestWindLevel;
        }
        else if (usedSize == size)
        {
            Collections.sort(state);
            Room last = state.get(0);
            if (last.requestWindLevel < windLevel)
            {
                last.state = 0;
                last.serveTime = 0;
                sendStop(last.roomId);

                room.state = 1;
                room.serveTime = 0;
                room.waitingTime = 0;
                state.add(room);
                return room.requestWindLevel;
            }
            else
            {
                room.state = 0;
                room.serveTime = 0;
                room.waitingTime = Schedule.TIME;
                state.add(room);
                return 0;
            }
        }
        else
        {
            room.waitingTime = Schedule.TIME;
            room.serveTime = 0;
            room.state = 0;
            state.add(room);
            return 0;
        }

    }
    public void releaseRoom(String roomId)
    {
        for (Room room:state)
        {
            if (room.roomId.equals(roomId))
            {
                usedSize --;
                state.remove(room);
                break;
            }
        }
        if (flag)
            nextRoom();
    }
    public void timeCost()
    {
        for (Room room:state)
        {
            if (room.state == 1)
                room.serveTime ++;
            else
                room.waitingTime --;
        }
    }
    public void report()
    {
        Collections.sort(state);
        System.out.println("serve: " + usedSize + "  waiting: " + (state.size()-usedSize));
        for (Room i:state)
        {
            System.out.println(i.toString());
        }
        System.out.println("----------------");
    }

//    public static void main(String[] args) {
//        Schedule schedule = new Schedule(3);
//        schedule.roomRequest("301",2);
//        schedule.roomRequest("302",2);
//        schedule.roomRequest("303",3);
//        schedule.roomRequest("304",1);
//        schedule.roomRequest("305",2);
//        schedule.run();
//    }
}
