
import pymysql
    from flask import Flask, jsonify, render_template, request
        from flask_cors import CORS
        from datetime import datetime
import json
        from flask.json.provider import JSONProvider


# ==============================================================================
# 1. JSON 커스텀 처리 (datetime 객체 직렬화)
# ==============================================================================
# Flask가 DB에서 가져온 날짜/시간(datetime) 객체를 JSON 문자열로 자동 변환하도록 설정
        class CustomJSONProvider(JSONProvider) :
        def dumps(self, obj, **kwargs) :
        return json.dumps(obj, **kwargs, default = self.default)
        def loads(self, s, **kwargs) :
        return json.loads(s, **kwargs)
        @staticmethod
        def default(o) :
        if isinstance(o, datetime) :
            return o.isoformat()
            return super(CustomJSONProvider, CustomJSONProvider).default(o)

# ==============================================================================
# 2. Flask 애플리케이션 초기화 및 DB 설정
# ==============================================================================
            app = Flask(__name__)
            app.json = CustomJSONProvider(app)
            CORS(app)

# 데이터베이스 접속 정보 설정
            app.config['MYSQL_HOST'] = '192.168.0.92'
            app.config['MYSQL_USER'] = 'mfcuser'
            app.config['MYSQL_PASSWORD'] = 'Moble1234'
            app.config['MYSQL_DB'] = 'themost_db'

# 데이터베이스 연결 객체를 생성하고 반환하는 함수
            def get_db_connection() :
            connection = pymysql.connect(
                host = app.config['MYSQL_HOST'],
                user = app.config['MYSQL_USER'],
                password = app.config['MYSQL_PASSWORD'],
                db = app.config['MYSQL_DB'],
                charset = 'utf8mb4',
                cursorclass = pymysql.cursors.DictCursor
            )
            return connection

# 기본 페이지 라우트 (GET /)
            @app.route('/')
            def index() :
            return render_template('test.html')

# ==============================================================================
# 3. API 라우트 정의
# ==============================================================================
            @app.route('/api/login', methods = ['POST'])
            def login() :
    # 사용자 로그인 처리: DB에서 username과 password를 확인하여 사용자 정보를 반환
    # 실패 시 401 Unauthorized 에러 반환
            conn = None
            try :
            data = request.get_json()
            username = data.get('username')
            password = data.get('password')
            conn = get_db_connection()
            with conn.cursor() as cursor :
    sql = "SELECT member_id, username, name, grade, address, phone FROM members WHERE username = %s AND password = %s"
        cursor.execute(sql, (username, password))
        user = cursor.fetchone()
        if user :
            return jsonify(user)
        else:
    return jsonify({ 'error': '아이디 또는 비밀번호가 일치하지 않습니다.' }), 401
        except Exception as e :
    print(f"Error during login: {e}")
        return jsonify({ 'error': '로그인 중 오류가 발생했습니다.' }), 500
        finally :
        if conn :
            conn.close()

# ------------------------------------------------------------------------------

# 주문 생성 및 재고 관리 (트랜잭션):
# 1. 재고를 확인하고 FOR UPDATE로 락(Lock)을 걸어 동시성 문제 방지
# 2. orders, order_details 테이블에 주문 정보 삽입
# 3. product_options 테이블에서 재고 감소
# 4. 오류 발생 시 전체 롤백(rollback) 처리
            @app.route('/api/order', methods = ['POST'])
           def create_order() :

            conn = None
            try :
            data = request.get_json()
            user = data.get('user')
            items = data.get('items')
            total_price = data.get('totalPrice')
            if not user or not items :
                return jsonify({ 'error': '주문 정보가 올바르지 않습니다.' }), 400
                conn = get_db_connection()
                conn.begin()
                with conn.cursor() as cursor :
    for item in items :
    cursor.execute("SELECT stock FROM product_options WHERE option_id = %s FOR UPDATE", (item['option_id'], ))
        result = cursor.fetchone()
        if not result or result['stock'] < item['quantity'] :
            conn.rollback()
            return jsonify({ 'error': f"재고 부족: {item['product_name']} ({item['color_name']}/{item['size_name']})" }), 400
            order_sql = "INSERT INTO orders (member_id, recipient_name, shipping_address, recipient_phone, total_price, status) VALUES (%s, %s, %s, %s, %s, '결제완료')"
            cursor.execute(order_sql, (user['member_id'], user['name'], user['address'], user['phone'], total_price))
            order_id = cursor.lastrowid
            for item in items :
    detail_sql = "INSERT INTO order_details (order_id, option_id, quantity, price_per_item) VALUES (%s, %s, %s, %s)"
        cursor.execute(detail_sql, (order_id, item['option_id'], item['quantity'], item['price']))
        update_stock_sql = "UPDATE product_options SET stock = stock - %s WHERE option_id = %s"
        cursor.execute(update_stock_sql, (item['quantity'], item['option_id']))
        conn.commit()
        return jsonify({ 'message': '주문이 성공적으로 완료되었습니다.', 'order_id' : order_id })
        except Exception as e :
    if conn : conn.rollback()
        print(f"Error creating order: {e}")
        return jsonify({ 'error': '주문 처리 중 오류가 발생했습니다.' }), 500
        finally :
        if conn : conn.close()

# ------------------------------------------------------------------------------

            @app.route('/api/orders', methods = ['POST'])
            def get_order_history() :

    # 주문 내역 조회: member_id를 받아 주문 정보와 상세 상품 목록을 조회
    # period_days를 통해 조회 기간 필터링 가능

            conn = None

            try :          
    # ... (DB 조인 쿼리를 실행하여 주문 및 상품 상세 정보 조회)
    # ... (조회된 결과를 주문 ID별로 그룹화하여 반환)     
            data = request.get_json()
            member_id = data.get('member_id')
            period_days = data.get('period_days', 0)
            if not member_id :
                return jsonify({ 'error': '사용자 정보가 필요합니다.' }), 400
                conn = get_db_connection()
                with conn.cursor() as cursor :
    sql = """
        SELECT o.order_id, o.order_date, o.total_price, o.status, od.quantity, od.price_per_item,
        p.product_name, c.color_name, s.size_name, pi.image_url
        FROM orders AS o
        JOIN order_details AS od ON o.order_id = od.order_id
        JOIN product_options AS po ON od.option_id = po.option_id
        JOIN products AS p ON po.product_id = p.product_id
        JOIN colors AS c ON po.color_id = c.color_id
        JOIN sizes AS s ON po.size_id = s.size_id
        LEFT JOIN product_images AS pi ON p.product_id = pi.product_id AND pi.is_main_image = TRUE
        WHERE o.member_id = % s
        """
        params = [member_id]
        if period_days > 0:
    sql += " AND o.order_date >= NOW() - INTERVAL %s DAY"
        params.append(period_days)
        sql += " ORDER BY o.order_date DESC, o.order_id DESC;"
        cursor.execute(sql, tuple(params))
        results = cursor.fetchall()
        orders_dict = {}
        for row in results :
    order_id = row['order_id']
        if order_id not in orders_dict :
    orders_dict[order_id] = { 'order_id': order_id, 'order_date' : row['order_date'], 'total_price' : row['total_price'], 'status' : row['status'], 'items' : [] }
        orders_dict[order_id]['items'].append({ 'product_name': row['product_name'], 'color_name' : row['color_name'], 'size_name' : row['size_name'], 'quantity' : row['quantity'], 'price_per_item' : row['price_per_item'], 'image_url' : row['image_url'] })
        return jsonify(list(orders_dict.values()))
        except Exception as e :
    print(f"Error fetching order history: {e}")
        return jsonify({ 'error': str(e) }), 500
        finally :
        if conn : conn.close()

            @app.route('/api/review/eligibility', methods = ['POST'])
            def check_review_eligibility() :
        # 리뷰 작성 가능 여부 확인:
        # 1. 해당 상품에 대해 이미 리뷰를 작성했는지 확인
        # 2. 7일 이내에 해당 상품을 구매했는지 확인
            conn = None
            try :
            data = request.get_json()
            member_id = data.get('member_id')
            product_id = data.get('product_id')
            if not all([member_id, product_id]) :
                return jsonify({ 'eligible': False, 'reason' : '필수 정보 누락' }), 400
                conn = get_db_connection()
                with conn.cursor() as cursor :
    cursor.execute("SELECT review_id FROM reviews WHERE member_id = %s AND product_id = %s", (member_id, product_id))
        if cursor.fetchone() :
            return jsonify({ 'eligible': False, 'reason' : '이미 리뷰를 작성했습니다.' })
            sql = "SELECT o.order_id FROM orders o JOIN order_details od ON o.order_id = od.order_id JOIN product_options po ON od.option_id = po.option_id WHERE o.member_id = %s AND po.product_id = %s AND o.order_date >= NOW() - INTERVAL 7 DAY LIMIT 1"
            cursor.execute(sql, (member_id, product_id))
            if cursor.fetchone() :
                return jsonify({ 'eligible': True })
            else:
    return jsonify({ 'eligible': False, 'reason' : '7일 이내 구매 내역이 없습니다.' })
        except Exception as e :
    print(f"Error checking review eligibility: {e}")
        return jsonify({ 'error': str(e) }), 500
        finally :
        if conn : conn.close()

# ------------------------------------------------------------------------------
            @app.route('/api/review', methods = ['POST'])
            def post_review() :
           # 리뷰 등록: 리뷰 작성 권한(7일 이내 구매 여부)을 재확인 후 reviews 테이블에 삽입
            conn = None
            try :
            data = request.get_json()
            member_id = data.get('member_id')
            product_id = data.get('product_id')
            rating = data.get('rating')
            content = data.get('content')
            if not all([member_id, product_id, rating, content]) :
                return jsonify({ 'error': '필수 정보가 누락되었습니다.' }), 400
                conn = get_db_connection()
                conn.begin()
                with conn.cursor() as cursor :
    cursor.execute("SELECT review_id FROM reviews WHERE member_id = %s AND product_id = %s", (member_id, product_id))
        if cursor.fetchone() :
            conn.rollback()
            return jsonify({ 'error': '이미 리뷰를 작성했습니다.' }), 403
            sql_check_purchase = "SELECT o.order_id FROM orders o JOIN order_details od ON o.order_id = od.order_id JOIN product_options po ON od.option_id = po.option_id WHERE o.member_id = %s AND po.product_id = %s AND o.order_date >= NOW() - INTERVAL 7 DAY LIMIT 1"
            cursor.execute(sql_check_purchase, (member_id, product_id))
            if not cursor.fetchone() :
                conn.rollback()
                return jsonify({ 'error': '리뷰 작성 권한이 없습니다.' }), 403
                sql_insert = "INSERT INTO reviews (member_id, product_id, rating, content, created_at) VALUES (%s, %s, %s, %s, NOW())"
                cursor.execute(sql_insert, (member_id, product_id, rating, content))
                conn.commit()
                return jsonify({ 'message': '리뷰가 성공적으로 등록되었습니다.' })
                except Exception as e :
    if conn : conn.rollback()
        print(f"Error posting review: {e}")
        return jsonify({ 'error': '리뷰 등록 중 오류가 발생했습니다.' }), 500
        finally :
        if conn : conn.close()

# ------------------------------------------------------------------------------
            @app.route('/api/qna/question', methods = ['POST'])
            def post_question() :
            # 상품 문의(Q&A) 질문 등록: qna 테이블에 문의 내용을 삽입
            conn = None
            try :
            data = request.get_json()
            product_id = data.get('product_id')
            member_id = data.get('member_id')
            content = data.get('content')
            is_private = data.get('is_private', False)
            if not all([product_id, member_id, content]) :
                return jsonify({ 'error': '필수 정보가 누락되었습니다.' }), 400
                conn = get_db_connection()
                with conn.cursor() as cursor :
    sql = "INSERT INTO qna (product_id, member_id, question_content, is_private, question_date) VALUES (%s, %s, %s, %s, NOW())"
        cursor.execute(sql, (product_id, member_id, content, is_private))
        conn.commit()
        return jsonify({ 'message': '문의가 성공적으로 등록되었습니다.' })
        except Exception as e :
    if conn : conn.rollback()
        print(f"Error posting question: {e}")
        return jsonify({ 'error': '문의 등록 중 오류가 발생했습니다.' }), 500
        finally :
        if conn : conn.close()

# ------------------------------------------------------------------------------
            @app.route('/api/qna/answer', methods = ['POST'])
            def post_answer() :
            # 상품 문의(Q&A) 답변 등록: 관리자 권한(`grade`가 '관리자')을 확인 후 답변 내용을 업데이트
            conn = None
            try :
            data = request.get_json()
            qna_id = data.get('qna_id')
            answer = data.get('answer')
            admin_id = data.get('admin_id')
            if not all([qna_id, answer, admin_id]) :
                return jsonify({ 'error': '필수 정보가 누락되었습니다.' }), 400
                conn = get_db_connection()
                with conn.cursor() as cursor :
    cursor.execute("SELECT grade FROM members WHERE member_id = %s", (admin_id, ))
        user = cursor.fetchone()
        if not user or user['grade'] != '관리자' :
            return jsonify({ 'error': '답변을 작성할 권한이 없습니다.' }), 403
            sql = "UPDATE qna SET answer_content = %s, answer_date = NOW() WHERE qna_id = %s"
            cursor.execute(sql, (answer, qna_id))
            conn.commit()
            return jsonify({ 'message': '답변이 성공적으로 등록되었습니다.' })
            except Exception as e :
    if conn : conn.rollback()
        print(f"Error posting answer: {e}")
        return jsonify({ 'error': '답변 등록 중 오류가 발생했습니다.' }), 500
        finally :
        if conn : conn.close()

# ------------------------------------------------------------------------------
            @app.route('/api/products', methods = ['GET'])
            def get_all_products() :
            # 전체 상품 목록 조회: 상품 정보와 함께 'new'(30일 이내), 'best'(조회수 500 초과) 태그를 동적으로 추가
            conn = None
            try :
            conn = get_db_connection()
            with conn.cursor() as cursor :
    sql = "SELECT p.*, b.brand_name, c.category_name, pi.image_url FROM products AS p LEFT JOIN brands AS b ON p.brand_id = b.brand_id LEFT JOIN categories AS c ON p.category_id = c.category_id LEFT JOIN product_images AS pi ON p.product_id = pi.product_id AND pi.is_main_image = TRUE ORDER BY p.created_at DESC;"
        cursor.execute(sql)
        products = cursor.fetchall()
        for p in products :
    p['tags'] = []
        if p['created_at'] and (datetime.now() - p['created_at']).days < 30 :
            p['tags'].append('new')
            if p['view_count'] > 500 :
                p['tags'].append('best')
                return jsonify(products)
                except Exception as e :
    print(f"Error fetching products: {e}")
        return jsonify({ 'error': str(e) }), 500
        finally :
        if conn :
            conn.close()

# ------------------------------------------------------------------------------
            @app.route('/api/product/<int:product_id>', methods = ['GET'])
            def get_product_detail(product_id) :
            # 상품 상세 정보 조회: 상품 기본 정보, 이미지, 옵션, 리뷰, Q&A를 모두 조회하여 하나의 JSON으로 반환
            conn = None
            try :
            conn = get_db_connection()
            with conn.cursor() as cursor :
    cursor.execute("SELECT p.*, b.brand_name FROM products p JOIN brands b ON p.brand_id = b.brand_id WHERE p.product_id = %s", (product_id, ))
        product = cursor.fetchone()
        if not product : return jsonify({ 'error': '상품을 찾을 수 없습니다.' }), 404
            cursor.execute("SELECT image_url FROM product_images WHERE product_id = %s ORDER BY is_main_image DESC", (product_id, ))
            images = cursor.fetchall()
            product['images'] = [img['image_url'] for img in images]
            sql_options = "SELECT po.option_id, po.stock, c.color_id, c.color_name, s.size_id, s.size_name FROM product_options po JOIN colors c ON po.color_id = c.color_id JOIN sizes s ON po.size_id = s.size_id WHERE po.product_id = %s;"
            cursor.execute(sql_options, (product_id, ))
            product['options'] = cursor.fetchall()
            cursor.execute("SELECT r.*, m.name as member_name FROM reviews r JOIN members m ON r.member_id = m.member_id WHERE r.product_id = %s ORDER BY r.created_at DESC", (product_id, ))
            product['reviews'] = cursor.fetchall()
            cursor.execute("SELECT q.*, m.name as member_name FROM qna q JOIN members m ON q.member_id = m.member_id WHERE q.product_id = %s ORDER BY q.question_date DESC", (product_id, ))
            product['qna'] = cursor.fetchall()
            return jsonify(product)
            except Exception as e :
    print(f"Error fetching product detail: {e}")
        return jsonify({ 'error': str(e) }), 500
        finally :
        if conn :
            conn.close()

# ------------------------------------------------------------------------------
            @app.route('/api/product/<int:product_id>/view', methods = ['POST'])
            def increment_view_count(product_id) :
            # 상품 조회수 증가: 해당 상품의 view_count를 1 증가시킴
            conn = None
            try :
            conn = get_db_connection()
            with conn.cursor() as cursor :
    sql = "UPDATE products SET view_count = view_count + 1 WHERE product_id = %s"
        cursor.execute(sql, (product_id, ))
        conn.commit()
        return jsonify({ 'message': 'View count updated successfully.' })
        except Exception as e :
    if conn : conn.rollback()
        print(f"Error incrementing view count: {e}")
        return jsonify({ 'error': '조회수 업데이트 중 오류가 발생했습니다.' }), 500
        finally :
        if conn :
            conn.close()

# ==============================================================================
# 4. 애플리케이션 실행
# ==============================================================================

            if __name__ == '__main__' :
                app.run(host = '0.0.0.0', port = 5000, debug = True, use_reloader = False)


