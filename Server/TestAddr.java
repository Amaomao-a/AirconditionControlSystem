/*
 * To change this template, choose Tools | Templates
 * and open the template in the editor.
 */

/**
 *
 * @author Administrator
 */
import java.net.InetAddress;
import java.net.NetworkInterface;
import java.net.SocketException;
import java.util.ArrayList;
import java.util.Enumeration;
import java.util.List;

public class TestAddr {

    /**
     * 获取本机所有IP
     */
    static String getAllLocalHostIP() {
        List<String> res = new ArrayList<String>();
        Enumeration netInterfaces;
        try {
            netInterfaces = NetworkInterface.getNetworkInterfaces();
            InetAddress ip = null;
            while (netInterfaces.hasMoreElements()) {
                NetworkInterface ni = (NetworkInterface) netInterfaces
                        .nextElement();
                //System.out.println("---Name---:" + ni.getName());
                Enumeration nii = ni.getInetAddresses();
                while (nii.hasMoreElements()) {
                    ip = (InetAddress) nii.nextElement();
                    if (ip.getHostAddress().indexOf(":") == -1) {
                        res.add(ip.getHostAddress());
                        //System.out.println("本机的ip=" + ip.getHostAddress());
                        String t = ip.getHostAddress();
                        if (!t.startsWith("192.168.")&&!t.startsWith("127.0."))
                            return t;
                    }
                }
            }
        } catch (SocketException e) {
            e.printStackTrace();
        }
        return "0.0.0.0";
    }
}