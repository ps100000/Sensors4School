package com.sensorsapp.school.sensors4school;

import android.graphics.Color;
import android.support.v7.app.AppCompatActivity;
import android.os.Bundle;

import com.androidplot.xy.LineAndPointFormatter;
import com.androidplot.xy.SimpleXYSeries;
import com.androidplot.xy.XYPlot;
import com.androidplot.xy.XYSeries;

import java.util.Arrays;

public class Graph3 extends AppCompatActivity {
        XYSeries XYseries3;
        XYPlot graph3;
        Thread3send startThread1 = new Thread3send();
        Thread3get startThread2 = new Thread3get();
    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_graph3);
        graph3 = (XYPlot)findViewById(R.id.graph3);
        Integer[]ArrayX3 = new Integer[]{0};
        Integer[]ArrayY3 = new Integer[]{0};
        XYseries3 = new SimpleXYSeries(Arrays.asList(ArrayX3),Arrays.asList(ArrayY3),"XYseries3");
        graph3.addSeries(XYseries3,new LineAndPointFormatter(Color.RED,Color.BLUE,null,null));
        new Thread(startThread1).start();
        new Thread(startThread2).start();

    }


    class Thread3send implements Runnable {

        @Override
        public void run() {

        }
    }
    class Thread3get implements Runnable {

        @Override
        public void run() {

        }
    }
}
