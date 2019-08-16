#!/usr/bin/env python
# -*- encoding: utf-8 -*-
import os
from flask import Flask, render_template, abort, jsonify
from models import DynamoDB

# Flaskのインスタンス生成
app = Flask(__name__)

# DynamoDBアクセス用のインスタンス生成
db = DynamoDB()

# cookieを暗号化する秘密鍵
app.config['SECRET_KEY'] = os.urandom(24)


@app.route('/')
def index():
    '''
    一覧画面
    '''
    # DynamoDBから一覧取得
    results = db.getlist_item()
    # HTML表示用意日時文字列の生成、dictへ追加
    results = db.adddictforList(results)

    return render_template('index.html', results=results)

@app.route('/view/<pk>')
def view(pk):
    '''
    結果参照処理
    '''
    # DynamoDBから日時で検索した結果取得
    result = db.search_item(pk)
    # HTML表示向けに画像ファイルが存在するか否か判定する
    result = db.checkforResult(result)

    return render_template('view.html', result=result)

@app.errorhandler(404)
def error_handler(error):
    '''
    abort(404) した時にレスポンスをレンダリングするハンドラ
    '''
    return render_template('notfound.html', data={'data' : error.description['data']}), error.code

if __name__ == '__main__':
    app.run(host='0.0.0.0')

