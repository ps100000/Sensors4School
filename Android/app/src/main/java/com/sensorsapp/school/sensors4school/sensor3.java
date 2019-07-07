package com.sensorsapp.school.sensors4school;

import android.content.Intent;
import android.support.v7.app.AppCompatActivity;
import android.os.Bundle;
import android.view.View;
import android.widget.Button;

public class sensor3 extends AppCompatActivity implements View.OnClickListener {

    Button Fertig3;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_sensor3);
        Fertig3 = (Button)findViewById(R.id.Fertig3);
        Fertig3.setOnClickListener(this);
    }

    @Override
    public void onClick(View v) {
        Intent Graph3 = new Intent (this, Graph3.class);
        startActivity(Graph3);
    }
}
