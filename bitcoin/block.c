#include "bitcoin/block.h"
#include "bitcoin/pullpush.h"
#include "bitcoin/tx.h"
#include <ccan/str/hex/hex.h>

#include "shadouble.h"
#include <stdio.h>

struct half_hdr {
    le32 version;
    struct sha256_double prev_hash;
    struct sha256_double merkle_hash;
    le32 timestamp;
    le32 target;
    le32 nonce;

    struct sha256_double hashStateRoot; // qtum
    struct sha256_double hashUTXORoot; // qtum

    struct sha256_double prev_stake_hash;
    le32 prev_stake_n;
};

/* Encoding is <blockhdr> <varint-num-txs> <tx>... */
struct bitcoin_block *bitcoin_block_from_hex(const tal_t *ctx,
					     const char *hex, size_t hexlen)
{
	struct bitcoin_block *b;
	u8 *linear_tx;
	const u8 *p;
	size_t len, i, num;

	if (hexlen && hex[hexlen-1] == '\n')
		hexlen--;

	/* Set up the block for success. */
	b = tal(ctx, struct bitcoin_block);

	/* De-hex the array. */
	len = hex_data_size(hexlen);
	p = linear_tx = tal_arr(ctx, u8, len);
	if (!hex_decode(hex, hexlen, linear_tx, len))
		return tal_free(b);

    struct half_hdr h;

    pull(&p, &len, &h.version, sizeof(h.version));
    pull(&p, &len, &h.prev_hash, sizeof(h.prev_hash));
    pull(&p, &len, &h.merkle_hash, sizeof(h.merkle_hash));
    pull(&p, &len, &h.timestamp, sizeof(h.timestamp));
    pull(&p, &len, &h.target, sizeof(h.target));
    pull(&p, &len, &h.nonce, sizeof(h.nonce));
    pull(&p, &len, &h.hashStateRoot, sizeof(h.hashStateRoot));
    pull(&p, &len, &h.hashUTXORoot, sizeof(h.hashUTXORoot));

    pull(&p, &len, &h.prev_stake_hash, sizeof(h.prev_stake_hash));
    pull(&p, &len, &h.prev_stake_n, sizeof(h.prev_stake_n));

    memcpy(&b->hdr, &h, sizeof(h));

    u8 xx = pull_varint(&p, &len);
    u8 * newP = tal_arr(b, u8, sizeof(h) + sizeof(xx) + xx);

    memcpy(newP, &h, sizeof(h));
    memcpy(newP + sizeof(h), &xx, sizeof(xx));

    pull(&p, &len, newP + sizeof(h) + sizeof(xx), xx);

    sha256_double(&b->block_id, newP, sizeof(h) + sizeof(xx) + xx);

	num = pull_varint(&p, &len);
	b->tx = tal_arr(b, struct bitcoin_tx *, num);
	for (i = 0; i < num; i++)
		b->tx[i] = pull_bitcoin_tx(b->tx, &p, &len);

	/* We should end up not overrunning, nor have extra */
	if (!p || len)
		return tal_free(b);

	tal_free(linear_tx);
	return b;
}

bool bitcoin_blkid_from_hex(const char *hexstr, size_t hexstr_len,
			    struct sha256_double *blockid)
{
	return bitcoin_txid_from_hex(hexstr, hexstr_len, blockid);
}

bool bitcoin_blkid_to_hex(const struct sha256_double *blockid,
			  char *hexstr, size_t hexstr_len)
{
	return bitcoin_txid_to_hex(blockid, hexstr, hexstr_len);
}
