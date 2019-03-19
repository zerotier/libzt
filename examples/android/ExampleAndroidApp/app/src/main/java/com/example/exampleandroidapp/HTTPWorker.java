package com.example.exampleandroidapp;

import com.zerotier.libzt.ZeroTierSocketFactory;
import com.zerotier.libzt.ZeroTier;

import java.io.ByteArrayOutputStream;
import java.io.InputStream;
import java.util.concurrent.ThreadLocalRandom;

import okhttp3.FormBody;
import okhttp3.OkHttpClient;
import okhttp3.Request;
import okhttp3.RequestBody;
import okhttp3.Response;

public class HTTPWorker extends Thread {

    @Override
    public void run() {
        long tid = Thread.currentThread().getId();
        // Test: Perform randomly-delayed HTTP GET requests
        if (true) {
            OkHttpClient.Builder builder = new OkHttpClient.Builder();
            builder.socketFactory(new ZeroTierSocketFactory());
            OkHttpClient client = builder.build();
            Request request1 = new Request.Builder()
                    .url("http://11.7.7.223:80/warandpeace.txt")
                    .build();
            Request request2 = new Request.Builder()
                    .url("http://11.7.7.223:8082/pumpkin.jpg")
                    .build();
            RequestBody formBody = new FormBody.Builder()
                    .add("message", "Your message")
                    .build();
            Request request3 = new Request.Builder()
                    .url("http://11.7.7.223:8082/")
                    .post(formBody)
                    .build();

            long i = 0;
            for (;;) {
                try {
                    int randomNum = ThreadLocalRandom.current().nextInt(0, 2 + 1);
                    i++;
                    Response response = null;
                    if (randomNum == 0) {
                        response = client.newCall(request1).execute();
                    }
                    if (randomNum == 1) {
                        //response = client.newCall(request2).execute();
                        response = client.newCall(request1).execute();
                    }
                    if (randomNum == 2) {
                        //response = client.newCall(request3).execute();
                        response = client.newCall(request1).execute();
                        //System.out.println(tid+"::POST");
                        //continue;
                    }
                    InputStream in = response.body().byteStream();
                    ByteArrayOutputStream buffer = new ByteArrayOutputStream();
                    int nRead;
                    byte[] data = new byte[16384];
                    while ((nRead = in.read(data, 0, data.length)) != -1) {
                        buffer.write(data, 0, nRead);
                    }
                    System.out.println(tid+"::GET: i="+i+", len="+buffer.toByteArray().length);

                } catch (Exception e) {
                    System.out.println(e);
                    e.printStackTrace();
                }

            }
        }
    }

}