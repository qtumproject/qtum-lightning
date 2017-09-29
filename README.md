## Getting Started

c-lightning currently only works on Linux (and possibly Mac OS with some tweaking), and requires a locally running `qtumd`.

### Installation

Please refer to the [installation documentation](doc/INSTALL.md) for detailed instructions.
For the impatient here's the gist of it for Ubuntu and Debian:

```
sudo apt-get install -y autoconf git build-essential libtool libgmp-dev libsqlite3-dev python python3
git clone https://gitlab.pixelplex.by/593_qtum/lightning.git
cd lightning
git checkout -b atbcoin origin/qtum
make
```
### Starting `lightningd`

In order to start `lightningd` you will need to have a local `qtumd` node running:

```
qtumd -daemon
```

Wait until `qtumd` has synchronized with the network.

You can start `lightningd` with the following command:

```
lightningd/lightningd --network=qtum
```
or
```
lightningd/lightningd --network=qtum --log-level=debug
```
for better informativeness.

### Opening a channel on the Qtum

First you need to transfer some funds to `lightningd` so that it can open a channel:

```
# Returns an address <address>
cli/lightning-cli newaddr 

# Returns a transaction id <txid>
qtum-cli sendtoaddress <address> <funds_amount>

# Retrieves the raw transaction <rawtx>
qtum-cli getrawtransaction <txid>

# Notifies `lightningd` that there are now funds available:
cli/lightning-cli addfunds <rawtx>
```

Eventually `lightningd` will include its own wallet making this transfer easier, but for now this is how it gets its funds.

Once `lightningd` has funds, we can connect to a node and open a channel.
Let's assume the remote node is accepting connections at `<recipient_ip>:<recipient_port>` and has the node ID `<recipient_id>`:

```
cli/lightning-cli connect <recipient_ip> <recipient_port> <recipient_id>
cli/lightning-cli fundchannel <recipient_id> <channel_amount>
```

This opens a connection and, on top of that connection, then opens a channel.
You can check the status of the channel using `cli/lightning-cli getpeers`.
The funding transaction needs to confirm in order for the channel to be usable, so wait generate 3 new blocks in qtum network, and once that is complete it `getpeers` should say that the status is in _CHANNELD_NORMAL_. Now, if we need we can close the channel.

### Receiving and receiving payments

Payments in Lightning are invoice based.
The recipient creates an invoice with the expected `<send_amount>` in millisatoshi and a `<label>`:

```
cli/lightning-cli invoice <send_amount> <label>
```

This returns a random value called `rhash` that is part of the invoice.
The recipient needs to communicate its ID `<recipient_id>`, `<rhash>` and the desired `<send_amount>` to the sender.

The sender needs to compute a route to the recipient, and use that route to actually send the payment.
The route contains the path that the payment will take throught the Lightning Network and the respective funds that each node will forward.
To the implementation `getroute` we should wait for generate atb network new 6 blocks after fulfilling `fundchannel`.

```
cli/lightning-cli getroute <recipient_id> <send_amount> 1
# Returns:
#{ "route" : [{ "id" : "<recipient_id>", "channel" : "<channel>", "msatoshi" : <send_amount>, "delay" : <delay> } ] }

cli/lightning-cli sendpay '[{ "id" : "<recipient_id>", "channel" : "<channel>", "msatoshi" : <send_amount>, "delay" : <delay> } ]' <rhash>
```

Notice that in the first step we stored the route in a variable and reused it in the second step.
`lightning-cli` should return a preimage that serves as a receipt, confirming that the payment was successful.
Upon receipt of preimage we close the channel following command:

```
daemon/lightning-cli close <recipient_id>
```

After close the channel, our state change to `ONCHAIND_MUTUAL` and after generate new 5 blocks in network we can close lightning node or make other different operations with it.

### Transfer of funds from `lightningd` to `Qtum`

If we want to transfer funds from `lightning` to `Qtum`, we can use `withdraw`. `<amount_to_Qtum>` in satoshi.

````
cli/lightning-cli withdraw <Qtum_address> <amount_to_Qtum>
```

After the commit `withdraw`, if successful, we get `<raw_transaction>` and `<txid>`.


## Further information

JSON-RPC interface is documented in the following manual pages:

* [invoice](doc/lightning-invoice.7.txt)
* [listinvoice](doc/lightning-listinvoice.7.txt)
* [waitinvoice](doc/lightning-waitinvoice.7.txt)
* [delinvoice](doc/lightning-delinvoice.7.txt)
* [getroute](doc/lightning-getroute.7.txt)
* [sendpay](doc/lightning-sendpay.7.txt)

For simple access to the JSON-RPC interface you can use the `cli/lightning-cli` tool, or the [python API client](contrib/pylightning).

