package com.example.exampleandroidapp;

import java.util.concurrent.ThreadLocalRandom;
import java.io.ByteArrayOutputStream;
import java.net.*;
import java.io.BufferedInputStream;
import java.io.IOException;
import java.io.InputStream;
import java.util.ArrayList;
import java.util.List;

import android.support.v7.app.AppCompatActivity;
import android.os.Bundle;

import okhttp3.OkHttpClient;
import okhttp3.Request;
import okhttp3.Response;
import okhttp3.MediaType;
import okhttp3.RequestBody;
import okhttp3.FormBody;

import com.zerotier.libzt.ZeroTier;
import com.zerotier.libzt.ZeroTierSocket;
import com.zerotier.libzt.ZeroTierSocketFactory;
import com.zerotier.libzt.ZeroTierSSLSocketFactory;
//import com.zerotier.libzt.ZeroTierEventListener;
import com.zerotier.libzt.ZeroTierSocketImplFactory;
import com.example.exampleandroidapp.MyZeroTierEventListener;

import java.io.BufferedReader;
import java.io.InputStreamReader;
//import java.net.HttpURLConnection;
import javax.net.ssl.HttpsURLConnection;
import java.net.URL;
import javax.net.ssl.SSLContext;

import com.squareup.picasso.Picasso;
import com.squareup.picasso.Callback;
import com.bumptech.glide.Glide;
import com.bumptech.glide.RequestBuilder;
import com.bumptech.glide.RequestManager;
import android.widget.ImageView;
import android.graphics.Bitmap;
import android.graphics.BitmapFactory;

import javax.net.ssl.HostnameVerifier;
import javax.net.ssl.HttpsURLConnection;
import javax.net.ssl.SSLContext;
import javax.net.ssl.SSLSession;
import javax.net.ssl.TrustManager;
import javax.net.ssl.X509TrustManager;
import java.security.cert.X509Certificate;

public class MainActivity extends AppCompatActivity {
    String requestURL = "http://11.7.7.223:80/";

    private static String createDataSize(int msgSize) {
        StringBuilder sb = new StringBuilder(msgSize);
        for (int i=0; i<msgSize; i++) {
            sb.append('a');
        }
        return sb.toString();
    }

    /*
    public static final MediaType JSON = MediaType.get("application/json; charset=utf-8");

    String HTTP_POST(String url, String json) throws IOException {
        RequestBody body = RequestBody.create(JSON, json);
        OkHttpClient.Builder builder = new OkHttpClient.Builder();
        builder.socketFactory(new ZeroTierSocketFactory());
        OkHttpClient client = builder.build();

        Request request = new Request.Builder()
                .url(url)
                .post(body)
                .build();
        try (Response response = client.newCall(request).execute()) {
            return response.body().string();
        }
    }

    void HTTP_GET()  throws IOException {
    }

    String sampleJson(String player1, String player2) {
        return "{'winCondition':'HIGH_SCORE',"
                + "'name':'Bowling',"
                + "'round':4,"
                + "'lastSaved':1367702411696,"
                + "'dateStarted':1367702378785,"
                + "'players':["
                + "{'name':'" + player1 + "','history':[10,8,6,7,8],'color':-13388315,'total':39},"
                + "{'name':'" + player2 + "','history':[6,10,5,10,10],'color':-48060,'total':41}"
                + "]}";
    }
*/

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);


        // WD TEST
        if (true) {
            System.out.println("Starting ZeroTier...");
            MyZeroTierEventListener listener = new MyZeroTierEventListener();
            ZeroTier.start(getApplicationContext().getFilesDir() + "/zerotier3", listener, 9994);

            while (listener.isOnline == false) {
                try {
                    Thread.sleep(100);
                } catch (Exception e) {
                }
            }

            System.out.println("joining network...");
            ZeroTier.join(0x0);
            System.out.println("waiting for callback");

            while (listener.isNetworkReady == false) {
                try {
                    Thread.sleep(100);
                } catch (Exception e) {
                }
            }

            //
            System.exit(0);
        }


        System.out.println("Starting ZeroTier...");
        MyZeroTierEventListener listener = new MyZeroTierEventListener();
        ZeroTier.start(getApplicationContext().getFilesDir() + "/zerotier3", listener, 9994);
        //System.out.println("ZTSDK nodeId=" + Long.toHexString(ZeroTier.get_node_id()));

        while (listener.isOnline == false) {
            try {
                Thread.sleep(100);
            } catch (Exception e) {
            }
        }

        System.out.println("joining network...");
        ZeroTier.join(0x0);
        System.out.println("waiting for callback");

        while (listener.isNetworkReady == false) {
            try {
                Thread.sleep(100);
            } catch (Exception e) {
            }
        }

        // Start threads
        List<Thread> threads = new ArrayList<>();
        for(int i = 0;i < 5;i++){
            try {
                Thread.sleep(500);
            } catch (Exception e) {}
            HTTPWorker thread = new HTTPWorker();
            thread.start();
            threads.add(thread);
        }

        System.out.println("sleeping...");
        try {
            Thread.sleep(60000000);
        } catch (Exception e) {}

        System.exit(0);

        // Test: setsockopt() and getsockopt() [non-exhaustive]
        /*
        if (true) {
            try {
                ZeroTierSocket zts = new ZeroTierSocket();
                zts.connect(new InetSocketAddress("11.7.7.223", 8082));
                zts.setSoTimeout(1337);
                int to = zts.getSoTimeout();
                assert to == 1337;

            } catch (Exception e) {

            }
        }
        */

        // Test: Perform randomly-delayed HTTP GET requests
        if (false) {
            OkHttpClient.Builder builder = new OkHttpClient.Builder();
            builder.socketFactory(new ZeroTierSocketFactory());
            OkHttpClient client = builder.build();
            Request request1 = new Request.Builder()
                    .url("http://11.7.7.223:8082/")
                    .build();

            long i = 0;
            for (;;) {
                try {
                    Thread.sleep((long)(Math.random() * 250));
                    Response response = client.newCall(request1).execute();
                    i++;
                    System.out.println("GET: i="+i+", len="+response.toString().length());
                    //response.close();

                } catch (Exception e) {
                    System.out.println(e);
                    System.exit(0);
                    e.printStackTrace();
                }

            }
        }

        // Test: Perform randomly-delayed HTTP GET requests
        if (true) {
            OkHttpClient.Builder builder = new OkHttpClient.Builder();
            builder.socketFactory(new ZeroTierSocketFactory());
            OkHttpClient client = builder.build();
            Request request1 = new Request.Builder()
                    .url("http://11.7.7.223:80/warandpeace.txt")
                    .header("Transfer-Encoding", "chunked")
                    //.header("Connection","close")
                    .build();

            Request request2 = new Request.Builder()
                    .url("http://11.7.7.223:8082/pumpkin.jpg")
                    .header("Transfer-Encoding", "chunked")
                    .header("Connection","close")
                    .build();

            String postMessage = "what?";
            try {
                postMessage = new String(createDataSize(1024).getBytes(), "UTF8");
            }
            catch (Exception e)
            {
                System.out.println(e);
            }

            MediaType JSON = MediaType.parse("application/json; charset=utf-8");
            okhttp3.RequestBody body = RequestBody.create(JSON, postMessage);

            //RequestBody formBody = new FormBody.Builder()
             //       .add("message", postMessage)
             //       .build();
            Request request3 = new Request.Builder()
                    .url("http://11.7.7.223:8082/")
                    //.retryOnConnectionFailure(true)
                    .header("Transfer-Encoding", "chunked")
                    .header("Connection","close")
                    //.header("Connection","keep-alive")
                    .post(body)
                    .build();

            long i = 0;
            for (;;) {
                try {
                    System.out.println("iteration="+i);
                    int randomNum = ThreadLocalRandom.current().nextInt(0, 2 + 1);
                    Thread.sleep(10);
                    i++;
                    Response response = null;
                    if (randomNum == 0) {
                        System.out.println("(0) GET: war");
                        response = client.newCall(request1).execute();

                    }
                    if (randomNum == 1) {
                        System.out.println("(1) GET: pumpkin");
                        response = client.newCall(request2).execute();

                    }
                    if (randomNum == 2) {
                        System.out.println("(2) POST: data");
                        response = client.newCall(request3).execute();
                        response.close();
                        continue;
                    }
                    InputStream in = response.body().byteStream();
                    ByteArrayOutputStream buffer = new ByteArrayOutputStream();
                    int nRead;
                    byte[] data = new byte[16384];
                    while ((nRead = in.read(data, 0, data.length)) != -1) {
                        buffer.write(data, 0, nRead);
                    }
                    System.out.println("GET: i="+i+", len="+buffer.toByteArray().length);
                    response.close();

                } catch (Exception e) {
                    System.out.println(e);
                    e.printStackTrace();
                }

            }
        }


        // Test: Perform randomly-delayed HTTP GET requests
        if (true) {
            OkHttpClient.Builder builder = new OkHttpClient.Builder();
            builder.socketFactory(new ZeroTierSocketFactory());
            OkHttpClient client = builder.build();

            Request request = new Request.Builder()
                    .url(requestURL+"warandpeace.txt")
                    .build();

            long i = 0;
            for (;;) {
                try {
                    //Thread.sleep((long)(Math.random()) );
                    //System.out.println("iteration="+i);
                    i++;
                    Response response = client.newCall(request).execute();
                    //System.out.println(response.body().string());

                    InputStream in = response.body().byteStream();
                    ByteArrayOutputStream buffer = new ByteArrayOutputStream();
                    int nRead;
                    byte[] data = new byte[16384];
                    while ((nRead = in.read(data, 0, data.length)) != -1) {
                        buffer.write(data, 0, nRead);
                    }

                    System.out.println("\tCOMPLETE, i="+i+", len="+buffer.toByteArray().length);


                    /*
                    //Bitmap bitmap = BitmapFactory.decodeByteArray(buffer.toByteArray(), 0, buffer.toByteArray().length);
                    //ImageView imageView = (ImageView) findViewById(R.id.imageView);
                    //imageView.setImageBitmap(bitmap);
                    */
                    /*
                    BufferedInputStream bufferedInputStream = new BufferedInputStream(in);
                    Bitmap bitmap=BitmapFactory.decodeStream(bufferedInputStream);
                    ImageView imageView = (ImageView) findViewById(R.id.imageView);
                    imageView.setImageBitmap(bitmap);
                    */

                    //
                    // Thread.sleep(5000);
                    //response.close();
                } catch (Exception e) {
                    System.out.println("EXCEPTION:"+e);
                    //System.exit(0);
                    e.printStackTrace();
                }

            }
        }



        // ...

        // Test #2: HttpURLConnection
        if (false)
        {


            // Create a trust manager that does not validate certificate chains
            TrustManager[] trustAllCerts = new TrustManager[] {new X509TrustManager() {
                public java.security.cert.X509Certificate[] getAcceptedIssuers() {
                    return null;
                }
                public void checkClientTrusted(X509Certificate[] certs, String authType) {
                }
                public void checkServerTrusted(X509Certificate[] certs, String authType) {
                }
            }
            };

            SSLContext context = null;

            // Set runtime default socket implementation factory
            try {
                //Socket.setSocketImplFactory(new ZeroTierSocketImplFactory());
                //SSLSocket.setSocketImplFactory(new ZeroTierSocketImplFactory());

                context = SSLContext.getInstance("SSL");
                //context.init(null, null, null);
                context.init(null, trustAllCerts, new java.security.SecureRandom());
                HttpsURLConnection.setDefaultSSLSocketFactory(context.getSocketFactory());
            } catch (Exception e) {
                e.printStackTrace();
            }

            // Create all-trusting host name verifier
            HostnameVerifier allHostsValid = new HostnameVerifier() {
                public boolean verify(String hostname, SSLSession session) {
                    return true;
                }
            };
            // Install the all-trusting host verifier
            HttpsURLConnection.setDefaultHostnameVerifier(allHostsValid);


            ZeroTierSSLSocketFactory customSocketFactory = new ZeroTierSSLSocketFactory(context.getSocketFactory());
            HttpsURLConnection.setDefaultSSLSocketFactory(customSocketFactory);




            // Test HTTP client
            String GET_URL = "https://10.6.6.152";
            String USER_AGENT = "Mozilla/5.0";
            try {
                URL obj = new URL(GET_URL);
                HttpsURLConnection con = (HttpsURLConnection) obj.openConnection();

                con.setSSLSocketFactory(customSocketFactory);

                con.setRequestMethod("GET");
                con.setRequestProperty("User-Agent", USER_AGENT);
                System.out.println("neat?");
                int responseCode = con.getResponseCode();
                System.out.println("GET Response Code :: " + responseCode);
                if (responseCode == HttpsURLConnection.HTTP_OK)
                {
                    // success
                    BufferedReader in = new BufferedReader(new InputStreamReader(con.getInputStream()));
                    String inputLine;
                    StringBuffer response = new StringBuffer();
                    while ((inputLine = in.readLine()) != null) {
                        response.append(inputLine);
                    }
                    in.close();
                    System.out.println(response.toString());
                } else {
                    System.out.println("GET request failed");
                }
            } catch (Exception e) {
                e.printStackTrace();
                System.out.println(e);
            }
        }
    }
}
