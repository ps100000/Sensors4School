package com.sensorsapp.school.sensors4school;

import android.content.Intent;
import android.support.v7.app.AppCompatActivity;
import android.os.Bundle;
import android.view.View;
import android.widget.ArrayAdapter;
import android.widget.Button;
import android.widget.Spinner;

public class sensor4 extends AppCompatActivity implements View.OnClickListener {
    Button Fertig4;
    Spinner dropdown1;
    Spinner dropdown2;
    Spinner dropdown3;
    String[] items1;
    String[] items2;
    String[] items3;
    ArrayAdapter<String> adapter1;
    ArrayAdapter<String> adapter2;
    ArrayAdapter<String> adapter3;
    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_sensor4);
        Fertig4 = (Button)findViewById(R.id.Fertig4);
        dropdown1 = findViewById(R.id.spinner);
        dropdown2 = findViewById(R.id.spinner2);
        dropdown3 = findViewById(R.id.spinner3);
        Fertig4.setOnClickListener(this);
        items1 = new String[]{"1", "2", "three"};
        items2 = new String[]{"1", "2", "three"};
        items3 = new String[]{"1", "2", "three"};
        adapter1 = new ArrayAdapter<>(this, android.R.layout.simple_spinner_dropdown_item, items1);
        adapter2 = new ArrayAdapter<>(this, android.R.layout.simple_spinner_dropdown_item, items2);
        adapter3 = new ArrayAdapter<>(this, android.R.layout.simple_spinner_dropdown_item, items3);
        dropdown1.setAdapter(adapter1);
        dropdown2.setAdapter(adapter2);
        dropdown3.setAdapter(adapter3);

    }

    @Override
    public void onClick(View v) {
        Intent Graph4 = new Intent(this, Graph4.class);
        startActivity(Graph4);
    }
}
