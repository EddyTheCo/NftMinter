# [NftMinter](https://eddytheco.github.io/NftMinter/wasm/index.html)

This repo produce a wasm application that can mint NFTs on the Shimmer network of IOTA.

In order to use the application one needs to set the address of the node to connect.
The Proof of Work has to be performed by the node (by setting the JWT for protected routes, by enabling PoW in the node...).
In principle it will also work for the shimmer mainnet by setting the node to a mainnet one(I have not tried).
This application is meant to be used on the testnet.
If using the mainnet **you are the ONLY responsible for the eventual loss of your funds**.


The application produce a random seed to be able to sign blocks using the 0 index address.
The application is not intended to store funds or nfts on that seed.
One could also save the produced seed just in case. 
Although you can set a seed  for recovering, **you should not input your personal seeds to the app**(read again the bold phrases).


I hope you find it useful.
If you like it and want to help on paying the education of my 2 future kids consider donating some iotas to the address below
or help me on the development. 

![](address.png) 



