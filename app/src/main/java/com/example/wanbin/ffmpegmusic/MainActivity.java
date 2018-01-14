package com.example.wanbin.ffmpegmusic;

import android.os.Bundle;
import android.os.Environment;
import android.support.v7.app.AppCompatActivity;
import android.view.SurfaceView;
import android.view.View;

import java.io.File;

public class MainActivity extends AppCompatActivity {

    // Used to load the 'native-lib' library on application startup.
    DavidPlayer davidPlayer;
    SurfaceView surfaceView;
    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);

        // Example of a call to a native method

        surfaceView = (SurfaceView) findViewById(R.id.surface);
        davidPlayer = new DavidPlayer();
        davidPlayer.setSurfaceView(surfaceView);
    }

    public void player(View view) {

        File file = new File(Environment.getExternalStorageDirectory(), "Warcraft3_End.avi");
        //  davidPlayer.playJava("rtmp://59.110.240.67/myapp/mystream");
        davidPlayer.playJava("rtmp://live.hkstv.hk.lxdns.com/live/hks");
       // davidPlayer.playJava("rtmp://114.67.145.163/myapp/mystream");
        //davidPlayer.playJava(file.getAbsolutePath());
    }


    public void onStop(View view) {
        davidPlayer.release();

    }


}
