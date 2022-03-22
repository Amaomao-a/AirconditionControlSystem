
import com.alibaba.fastjson.JSONObject;

import java.io.*;

public class LocalData
{
    private static String path = "data.json";
    public static String readJsonFile(String fileName) {
        String jsonStr = "";
        try {
            File jsonFile = new File(fileName);
            FileReader fileReader = new FileReader(jsonFile);
            Reader reader = new InputStreamReader(new FileInputStream(jsonFile),"utf-8");
            int ch = 0;
            StringBuffer sb = new StringBuffer();
            while ((ch = reader.read()) != -1) {
                sb.append((char) ch);
            }
            fileReader.close();
            reader.close();
            jsonStr = sb.toString();
            return jsonStr;
        } catch (IOException e) {
            e.printStackTrace();
            return null;
        }
    }
    public static boolean writeJsonFile(String fileName,String string) {
        try {
            File jsonFile = new File(fileName);
            FileWriter fileWriter = new FileWriter(jsonFile);
            fileWriter.write(string);
            fileWriter.close();
            return true;
        } catch (IOException e) {
            e.printStackTrace();
            return false;
        }
    }
    public static JSONObject getData()
    {
        return JSONObject.parseObject(readJsonFile(path));
    }
//    public static void main(String[] args) {
//        JSONObject json = new JSONObject();
//        json.put("mode",0);
//        json.put("size",3);
//        json.put("tempLow",17.0);
//        json.put("tempHigh",26.0);
//        double fee = 1.0 / 60;
//        json.put("rateFee",fee);
//        json.put("defaultWind",2);
//        writeJsonFile(path,json.toString());
//        System.out.println(getData().toString());
//    }
}
