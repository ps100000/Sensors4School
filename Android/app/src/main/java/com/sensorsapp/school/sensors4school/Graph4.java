package com.sensorsapp.school.sensors4school;

import android.graphics.Color;
import android.os.Handler;
import android.support.v7.app.AppCompatActivity;
import android.os.Bundle;
import android.util.Log;
import android.widget.TextView;

import com.androidplot.xy.LineAndPointFormatter;
import com.androidplot.xy.SimpleXYSeries;
import com.androidplot.xy.XYPlot;
import com.androidplot.xy.XYSeries;

import java.io.IOException;
import java.net.DatagramPacket;
import java.net.DatagramSocket;
import java.net.InetAddress;
import java.net.SocketException;
import java.net.UnknownHostException;
import java.nio.ByteBuffer;
import java.nio.ByteOrder;
import java.util.Arrays;

public class Graph4 extends AppCompatActivity {
    XYSeries XYseries4;
    XYPlot graph4;
    Thread4send startThread1 = new Thread4send();
    Thread4get startThread2 = new Thread4get();
    Boolean keepRunning, keepRunning2;
    DatagramSocket Dsocket4;
    String recivetxt;
    TextView textView;
    DatagramSocket DSock;
    DatagramPacket DPack;
    Handler Handler1 = new Handler();
    ByteBuffer byteBuffer1;
    int Ticks, PackageID, PackageType, datalenght, sensortype, semplingrate, NumberValues;
    int[] Values = new int[64], timestamp = new int[64];
    byte[] Buffer4, temp;
    int x = 0;
    InetAddress IAdress, selfIAdress;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_graph4);
        textView = (TextView) findViewById(R.id.textView);
        graph4 = (XYPlot) findViewById(R.id.graph4);
        Integer[]ArrayX4 = new Integer[]{0};
        Integer[]ArrayY4 = new Integer[]{0};
        XYseries4 = new SimpleXYSeries(Arrays.asList(ArrayX4),Arrays.asList(ArrayY4),"XYseries3");
        graph4.addSeries(XYseries4,new LineAndPointFormatter(Color.WHITE,Color.BLUE,null,null));
        new Thread(startThread2).start();
        temp = "1334".getBytes();
        try {
            IAdress = InetAddress.getByName("192.168.4.1");
            selfIAdress = InetAddress.getByName("192.168.4.2");
        } catch (UnknownHostException e) {
            e.printStackTrace();
        }
        new Thread(startThread1).start();
    }




    class Thread4send implements Runnable {

        @Override
        public void run() {
            keepRunning2 = true;
            while (keepRunning2) {
                try {
                    DSock = new DatagramSocket(1235);
                    DPack = new DatagramPacket(temp,temp.length,IAdress,1234);
                } catch (SocketException e) {
                    e.printStackTrace();
                }
                keepRunning2 = false;
            }

        }
    }
    class Thread4get implements Runnable {

        @Override
        public void run() {
            keepRunning = true;
            try {
                Dsocket4 = new DatagramSocket(1234, selfIAdress);
            } catch (SocketException e) {
                e.printStackTrace();
            }

            try {
                Dsocket4.send(new DatagramPacket("1234".getBytes(), "1234".getBytes().length, InetAddress.getByName("192.168.4.1"), 1234));
            } catch (IOException e) {
                e.printStackTrace();
            }

            while (keepRunning) {

                Buffer4 = new byte[4096];
                DatagramPacket Paket4 = new DatagramPacket(Buffer4,Buffer4.length);

                try {

                    Log.d("DEBUG", "hier geht los");
                    Dsocket4.receive(Paket4);
                    Log.d("Debug","etwas bekommen");
                    getLongData();
                    Log.d("DEBUG", "hier geht nicht los");
                    Handler1.post(new Runnable(){
                    @Override
                    public void run() {
                        textView.setText(recivetxt);
                        }
                    });
                } catch (IOException e) {
                    e.printStackTrace();
                }
                for(int i = 0; NumberValues > i; i++) {

                    ((SimpleXYSeries) XYseries4).addLast(++x,Values[i]);
                    if(XYseries4.size() > 4096){
                        ((SimpleXYSeries) XYseries4).removeFirst();
                    }
                }
                graph4.redraw();

            }
        }
        public void getLongData(){
            byteBuffer1 = ByteBuffer.wrap(Buffer4);
            byteBuffer1.order(ByteOrder.LITTLE_ENDIAN);
            Ticks = byteBuffer1.getInt(0);
            PackageID = byteBuffer1.getInt(4);
            PackageType = byteBuffer1.getInt(8);
            datalenght = byteBuffer1.getInt(12);
            sensortype = byteBuffer1.getInt(16);
            semplingrate = byteBuffer1.getInt(20);
            NumberValues = byteBuffer1.getInt(24);
            for(int i = 0; NumberValues > i; i++) {
                Values[i] = byteBuffer1.getInt(28+i*8);
                timestamp[i] = byteBuffer1.getInt(32+i*8);
            }
        }
    }
}
