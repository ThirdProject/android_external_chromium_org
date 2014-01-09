// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chromoting;

import android.accounts.Account;
import android.accounts.AccountManager;
import android.accounts.AccountManagerCallback;
import android.accounts.AccountManagerFuture;
import android.accounts.AuthenticatorException;
import android.accounts.OperationCanceledException;
import android.app.Activity;
import android.app.ProgressDialog;
import android.content.DialogInterface;
import android.content.Intent;
import android.content.SharedPreferences;
import android.os.Bundle;
import android.os.Handler;
import android.os.HandlerThread;
import android.util.Log;
import android.view.Menu;
import android.view.MenuItem;
import android.widget.ArrayAdapter;
import android.widget.ListView;
import android.widget.TextView;
import android.widget.Toast;

import org.chromium.chromoting.jni.JniInterface;
import org.json.JSONArray;
import org.json.JSONException;
import org.json.JSONObject;

import java.io.IOException;
import java.net.URL;
import java.net.URLConnection;
import java.util.Scanner;

/**
 * The user interface for querying and displaying a user's host list from the directory server. It
 * also requests and renews authentication tokens using the system account manager.
 */
public class Chromoting extends Activity implements JniInterface.ConnectionListener {
    /** Only accounts of this type will be selectable for authentication. */
    private static final String ACCOUNT_TYPE = "com.google";

    /** Scopes at which the authentication token we request will be valid. */
    private static final String TOKEN_SCOPE = "oauth2:https://www.googleapis.com/auth/chromoting " +
            "https://www.googleapis.com/auth/googletalk";

    /** Path from which to download a user's host list JSON object. */
    private static final String HOST_LIST_PATH =
            "https://www.googleapis.com/chromoting/v1/@me/hosts?key=";

    /** Lock to protect |mAccount| and |mToken|. */
    // TODO(lambroslambrou): |mHosts| needs to be protected as well.
    private Object mLock = new Object();

    /** User's account details. */
    private Account mAccount;

    /** Account auth token. */
    private String mToken;

    /** List of hosts. */
    private JSONArray mHosts;

    /** Refresh button. */
    private MenuItem mRefreshButton;

    /** Account switcher. */
    private MenuItem mAccountSwitcher;

    /** Greeting at the top of the displayed list. */
    private TextView mGreeting;

    /** Host list as it appears to the user. */
    private ListView mList;

    /** Callback handler to be used for network operations. */
    private Handler mNetwork;

    /** Dialog for reporting connection progress. */
    private ProgressDialog mProgressIndicator;

    /**
     * Called when the activity is first created. Loads the native library and requests an
     * authentication token from the system.
     */
    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.main);

        // Get ahold of our view widgets.
        mGreeting = (TextView)findViewById(R.id.hostList_greeting);
        mList = (ListView)findViewById(R.id.hostList_chooser);

        // Bring native components online.
        JniInterface.loadLibrary(this);

        // Thread responsible for downloading/displaying host list.
        HandlerThread thread = new HandlerThread("auth_callback");
        thread.start();
        mNetwork = new Handler(thread.getLooper());

        SharedPreferences prefs = getPreferences(MODE_PRIVATE);
        if (prefs.contains("account_name") && prefs.contains("account_type")) {
            // Perform authentication using saved account selection.
            mAccount = new Account(prefs.getString("account_name", null),
                    prefs.getString("account_type", null));
            AccountManager.get(this).getAuthToken(mAccount, TOKEN_SCOPE, null, this,
                    new HostListDirectoryGrabber(this), mNetwork);
            if (mAccountSwitcher != null) {
                mAccountSwitcher.setTitle(mAccount.name);
            }
        } else {
            // Request auth callback once user has chosen an account.
            Log.i("auth", "Requesting auth token from system");
            AccountManager.get(this).getAuthTokenByFeatures(
                    ACCOUNT_TYPE,
                    TOKEN_SCOPE,
                    null,
                    this,
                    null,
                    null,
                    new HostListDirectoryGrabber(this),
                    mNetwork
                );
        }
    }

    /** Called when the activity is finally finished. */
    @Override
    public void onDestroy() {
        super.onDestroy();
        JniInterface.disconnectFromHost();
    }

    /** Called to initialize the action bar. */
    @Override
    public boolean onCreateOptionsMenu(Menu menu) {
        getMenuInflater().inflate(R.menu.chromoting_actionbar, menu);
        mRefreshButton = menu.findItem(R.id.actionbar_directoryrefresh);
        mAccountSwitcher = menu.findItem(R.id.actionbar_accountswitcher);

        Account[] usableAccounts = AccountManager.get(this).getAccountsByType(ACCOUNT_TYPE);
        if (usableAccounts.length == 1 && usableAccounts[0].equals(mAccount)) {
            // If we're using the only available account, don't offer account switching.
            // (If there are *no* accounts available, clicking this allows you to add a new one.)
            mAccountSwitcher.setEnabled(false);
        }

        if (mAccount == null) {
            // If no account has been chosen, don't allow the user to refresh the listing.
            mRefreshButton.setEnabled(false);
        } else {
            // If the user has picked an account, show its name directly on the account switcher.
            mAccountSwitcher.setTitle(mAccount.name);
        }

        return super.onCreateOptionsMenu(menu);
    }

    /** Called whenever an action bar button is pressed. */
    @Override
    public boolean onOptionsItemSelected(MenuItem item) {
        if (item == mAccountSwitcher) {
            // The account switcher triggers a listing of all available accounts.
            AccountManager.get(this).getAuthTokenByFeatures(
                    ACCOUNT_TYPE,
                    TOKEN_SCOPE,
                    null,
                    this,
                    null,
                    null,
                    new HostListDirectoryGrabber(this),
                    mNetwork
                );
        }
        else {
            // The refresh button simply makes use of the currently-chosen account.
            AccountManager.get(this).getAuthToken(mAccount, TOKEN_SCOPE, null, this,
                    new HostListDirectoryGrabber(this), mNetwork);
        }

        return true;
    }

    /** Called when the user taps on a host entry. */
    public void connectToHost(JSONObject host) {
        try {
            synchronized (mLock) {
                JniInterface.connectToHost(mAccount.name, mToken, host.getString("jabberId"),
                        host.getString("hostId"), host.getString("publicKey"), this);
            }
        } catch (JSONException ex) {
            Log.w("host", ex);
            Toast.makeText(this, getString(R.string.error_reading_host),
                    Toast.LENGTH_LONG).show();
            // Close the application.
            finish();
        }
    }

    /**
     * Processes the authentication token once the system provides it. Once in possession of such a
     * token, attempts to request a host list from the directory server. In case of a bad response,
     * this is retried once in case the system's cached auth token had expired.
     */
    private class HostListDirectoryGrabber implements AccountManagerCallback<Bundle> {
        // TODO(lambroslambrou): Refactor this class to provide async interface usable on the UI
        // thread.

        /** Whether authentication has already been attempted. */
        private boolean mAlreadyTried;

        /** Communication with the screen. */
        private Activity mUi;

        /** Constructor. */
        public HostListDirectoryGrabber(Activity ui) {
            mAlreadyTried = false;
            mUi = ui;
        }

        /**
         * Retrieves the host list from the directory server. This method performs
         * network operations and must be run an a non-UI thread.
         */
        @Override
        public void run(AccountManagerFuture<Bundle> future) {
            Log.i("auth", "User finished with auth dialogs");
            try {
                // Here comes our auth token from the Android system.
                Bundle result = future.getResult();
                String accountName = result.getString(AccountManager.KEY_ACCOUNT_NAME);
                String accountType = result.getString(AccountManager.KEY_ACCOUNT_TYPE);
                String authToken = result.getString(AccountManager.KEY_AUTHTOKEN);
                Log.i("auth", "Received an auth token from system");

                synchronized (mLock) {
                    mAccount = new Account(accountName, accountType);
                    mToken = authToken;
                    getPreferences(MODE_PRIVATE).edit().putString("account_name", accountName).
                            putString("account_type", accountType).apply();
                }

                // Send our HTTP request to the directory server.
                URLConnection link =
                        new URL(HOST_LIST_PATH + JniInterface.nativeGetApiKey()).openConnection();
                link.addRequestProperty("client_id", JniInterface.nativeGetClientId());
                link.addRequestProperty("client_secret", JniInterface.nativeGetClientSecret());
                link.setRequestProperty("Authorization", "OAuth " + authToken);

                // Listen for the server to respond.
                StringBuilder response = new StringBuilder();
                Scanner incoming = new Scanner(link.getInputStream());
                Log.i("auth", "Successfully authenticated to directory server");
                while (incoming.hasNext()) {
                    response.append(incoming.nextLine());
                }
                incoming.close();

                // Interpret what the directory server told us.
                JSONObject data = new JSONObject(String.valueOf(response)).getJSONObject("data");
                mHosts = data.getJSONArray("items");
                Log.i("hostlist", "Received host listing from directory server");
            } catch (RuntimeException ex) {
                // Make sure any other failure is reported to the user (as an unknown error).
                throw ex;
            } catch (Exception ex) {
                // Assemble error message to display to the user.
                String explanation = getString(R.string.error_unknown);
                if (ex instanceof OperationCanceledException) {
                    explanation = getString(R.string.error_auth_canceled);
                } else if (ex instanceof AuthenticatorException) {
                    explanation = getString(R.string.error_no_accounts);
                } else if (ex instanceof IOException) {
                    if (!mAlreadyTried) {
                        // This was our first connection attempt.

                        synchronized (mLock) {
                            if (mAccount != null) {
                                // We got an account, but couldn't log into it. We'll retry in case
                                // the system's cached authentication token had already expired.
                                AccountManager authenticator = AccountManager.get(mUi);
                                mAlreadyTried = true;

                                Log.w("auth", "Requesting renewal of rejected auth token");
                                authenticator.invalidateAuthToken(mAccount.type, mToken);
                                mToken = null;
                                authenticator.getAuthToken(
                                        mAccount, TOKEN_SCOPE, null, mUi, this, mNetwork);

                                // We're not in an error state *yet*.
                                return;
                            }
                        }

                        // We didn't even get an account, so the auth server is likely unreachable.
                        explanation = getString(R.string.error_bad_connection);
                    } else {
                        // Authentication truly failed.
                        Log.e("auth", "Fresh auth token was also rejected");
                        explanation = getString(R.string.error_auth_failed);
                    }
                } else if (ex instanceof JSONException) {
                    explanation = getString(R.string.error_unexpected_response);
                }

                mHosts = null;
                Log.w("auth", ex);
                Toast.makeText(mUi, explanation, Toast.LENGTH_LONG).show();
            }

            // Share our findings with the user.
            runOnUiThread(new Runnable() {
                    @Override
                    public void run() {
                        updateUi();
                    }
            });
        }
    }

    /**
     * Updates the infotext and host list display.
     * This method affects the UI and must be run on the main thread.
     */
    private void updateUi() {
        synchronized (mLock) {
            mRefreshButton.setEnabled(mAccount != null);
            if (mAccount != null) {
                mAccountSwitcher.setTitle(mAccount.name);
            }
        }

        if (mHosts == null) {
            mGreeting.setText(getString(R.string.inst_empty_list));
            mList.setAdapter(null);
            return;
        }

        mGreeting.setText(getString(R.string.inst_host_list));

        ArrayAdapter<JSONObject> displayer = new HostListAdapter(this, R.layout.host);
        Log.i("hostlist", "About to populate host list display");
        try {
            int index = 0;
            while (!mHosts.isNull(index)) {
                displayer.add(mHosts.getJSONObject(index));
                ++index;
            }
            mList.setAdapter(displayer);
        } catch (JSONException ex) {
            Log.w("hostlist", ex);
            Toast.makeText(this, getString(R.string.error_cataloging_hosts),
                    Toast.LENGTH_LONG).show();

            // Close the application.
            finish();
        }
    }

    @Override
    public void onConnectionState(JniInterface.ConnectionListener.State state,
            JniInterface.ConnectionListener.Error error) {
        String stateText = getResources().getStringArray(R.array.protoc_states)[state.value()];
        boolean dismissProgress = false;
        switch (state) {
            case INITIALIZING:
            case CONNECTING:
            case AUTHENTICATED:
                // The connection is still being established, so we'll report the current progress.
                if (mProgressIndicator == null) {
                    mProgressIndicator = ProgressDialog.show(this,
                            getString(R.string.progress_title), stateText, true, true,
                            new DialogInterface.OnCancelListener() {
                                @Override
                                public void onCancel(DialogInterface dialog) {
                                    JniInterface.disconnectFromHost();
                                }
                            });
                } else {
                    mProgressIndicator.setMessage(stateText);
                }
                break;

            case CONNECTED:
                dismissProgress = true;
                Toast.makeText(this, stateText, Toast.LENGTH_SHORT).show();

                // Display the remote desktop.
                startActivityForResult(new Intent(this, Desktop.class), 0);
                break;

            case FAILED:
                dismissProgress = true;
                Toast.makeText(this, stateText + ": "
                        + getResources().getStringArray(R.array.protoc_errors)[error.value()],
                        Toast.LENGTH_LONG).show();

                // Close the Desktop view, if it is currently running.
                finishActivity(0);
                break;

            case CLOSED:
                // No need to show toast in this case. Either the connection will have failed
                // because of an error, which will trigger toast already. Or the disconnection will
                // have been initiated by the user.
                dismissProgress = true;
                finishActivity(0);
                break;
        }

        if (dismissProgress && mProgressIndicator != null) {
            mProgressIndicator.dismiss();
            mProgressIndicator = null;
        }
    }
}
