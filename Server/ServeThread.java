import java.util.List;

import static java.lang.Thread.sleep;

public class ServeThread implements Runnable
{
    private int seq;
    private int serveTime;
    private List<Room> state;
    public ServeThread(int seq,int serveTime,List<Room> state)
    {
        this.seq = seq;
        this.serveTime = serveTime;
        this.state = state;
        state.get(seq).state = 1;
    }
    @Override
    public void run()
    {

        while (true)
        {
            if (serveTime == 0)
            {
                state.get(seq).state=0;
                break;
            }
            try {
                sleep(1000);
            } catch (InterruptedException e) {
                e.printStackTrace();
            }
            serveTime --;
        }
    }
}
