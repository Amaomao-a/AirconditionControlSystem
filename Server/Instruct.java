import java.util.HashMap;

public class Instruct
{
    private String version;
    private String refId;
    private String handler;
    private String token;
    private HashMap<String,String> data;
    public String getVersion()
    {
        return version;
    }

    public String getRefId()
    {
        return refId;
    }

    public String getHandler()
    {
        return handler;
    }

    public String getToken()
    {
        return token;
    }

    public HashMap<String, String> getData()
    {
        return data;
    }

    public void setVersion(String version)
    {
        this.version = version;
    }

    public void setRefId(String refId)
    {
        this.refId = refId;
    }

    public void setHandler(String handler)
    {
        this.handler = handler;
    }

    public void setToken(String token)
    {
        this.token = token;
    }

    public void setData(HashMap<String, String> data)
    {
        this.data = data;
    }

    @Override
    public String toString()
    {
        return "Instruct{" +
                "version='" + version + '\'' +
                ", refId='" + refId + '\'' +
                ", handler='" + handler + '\'' +
                ", token='" + token + '\'' +
                ", data=" + data.toString() +
                '}';
    }

    public Instruct()
    {
    }
}
