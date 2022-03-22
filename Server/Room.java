import java.util.Comparator;

public class Room implements Comparable<Room>
{
    public String roomId;
    public int requestWindLevel;
    public int state;
    public int serveTime;
    public int waitingTime;
    public Room(String roomId,int requestWindLevel)
    {
        this.roomId = roomId;
        state = 0;
        serveTime = 0;
        this.requestWindLevel = requestWindLevel;
        this.waitingTime = 0;
    }

    @Override
    public int compareTo(Room room)
    {
        if (this.state > room.state)
            return -1;
        else if (this.state < room.state)
            return 1;
        if (this.requestWindLevel < room.requestWindLevel)
            return -1;
        else if (this.requestWindLevel > room.requestWindLevel)
            return 1;
        if (this.serveTime < room.serveTime)
            return 1;
        else if (this.serveTime > room.serveTime)
            return -1;
        return 0;
    }

    @Override
    public String toString() {
        return "Room{" +
                "roomId='" + roomId + '\'' +
                ", requestWindLevel=" + requestWindLevel +
                ", state=" + state +
                ", serveTime=" + serveTime +
                ", waitingTime=" + waitingTime +
                '}';
    }
}
