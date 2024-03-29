/*
 Copyright (c) Facebook, Inc. and its affiliates.
 All rights reserved.

 This source code is licensed under the BSD-style license found in the
 LICENSE file in the root directory of this source tree.
*/

package indexer_test

import (
	"testing"

	"magma/orc8r/cloud/go/services/state"
	"magma/orc8r/cloud/go/services/state/indexer"

	"github.com/stretchr/testify/assert"
)

const (
	imsi0 = "some_imsi_0"
	imsi1 = "some_imsi_1"
	type0 = "some_type_0"
	type1 = "some_type_1"
)

var emptyMatch []string

func TestSubscription_Match(t *testing.T) {
	sub := indexer.Subscription{Type: type0}

	id00 := state.ID{DeviceID: imsi0, Type: type0}
	id10 := state.ID{DeviceID: imsi1, Type: type0}
	id01 := state.ID{DeviceID: imsi0, Type: type1}
	id11 := state.ID{DeviceID: imsi1, Type: type1}

	// Match all
	sub.KeyMatcher = indexer.MatchAll
	assert.True(t, sub.Match(id00))
	assert.True(t, sub.Match(id10))
	assert.False(t, sub.Match(id01))
	assert.False(t, sub.Match(id11))

	// Match exact
	sub.KeyMatcher = indexer.NewMatchExact(imsi0)
	assert.True(t, sub.Match(id00))
	assert.False(t, sub.Match(id10))
	assert.False(t, sub.Match(id01))
	assert.False(t, sub.Match(id11))
}

func TestMatchAll_Match(t *testing.T) {
	m := indexer.MatchAll
	yesMatch := []string{"", "f", "fo", "foo", "foobar", "foobarbaz", "foobar !@#$%^&*()_+"}
	testMatchImpl(t, m, yesMatch, emptyMatch)
}

func TestMatchExact_Match(t *testing.T) {
	m := indexer.NewMatchExact("foobar")
	yesMatch := []string{"foobar"}
	noMatch := []string{"", "f", "fo", "foo", "foobarbaz", "foobar !@#$%^&*()_+"}
	testMatchImpl(t, m, yesMatch, noMatch)
}

func TestMatchPrefix_Match(t *testing.T) {
	m := indexer.NewMatchPrefix("foo")
	yesMatch := []string{"foo", "foobar", "foobarbaz", "foobar !@#$%^&*()_+"}
	noMatch := []string{"", "f", "fo"}
	testMatchImpl(t, m, yesMatch, noMatch)
}

func testMatchImpl(t *testing.T, m indexer.KeyMatcher, yesMatch, noMatch []string) {
	for _, str := range yesMatch {
		assert.True(t, m.Match(str))
	}
	for _, str := range noMatch {
		assert.False(t, m.Match(str))
	}
}
